#pragma once

#include <netron/common.hpp>
#include <netron/asio.hpp>
#include <netron/tsqueue.hpp>
#include <netron/message.hpp>
#include <netron/config.hpp>

namespace netron 
{

  // Forward declare
  template<typename T>
  class server_interface;

  template<typename T>
  class connection : public std::enable_shared_from_this<connection<T>>
  {
  public:
    enum class owner
    {
      server,
      client
    };

    connection(owner parent, asio::io_context& asio_context, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& messages_in, config_view owner_config)
      : m_asio_context(asio_context), m_socket(std::move(socket)), m_messages_in(messages_in), m_owner_config(owner_config)
    {
      m_owner_type = parent;

      if (m_owner_type == owner::server)
      {
        // Naive implementation. It only protects against accidental connections.
        m_handshake_out = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());

        // Precompute the handshake response
        m_handshake_check = scramble(m_handshake_out);
      }
      else
      {
        m_handshake_in = 0;
        m_handshake_out = 0;
      }
    }

    virtual ~connection()
    {}

    uint32_t get_id() const
    {
      return m_id;
    }

    auto get_endpoint() const
    {
      return m_socket.remote_endpoint();
    }

    const auto& get_config() const
    {
      return m_remote_config;
    }

    void connect_to_client(server_interface<T>* server, uint32_t uid = 0)
    {
      if (m_owner_type == owner::server)
      {
        if (m_socket.is_open())
        {
          m_id = uid;
          write_validation();
          read_validation(server);
        }
      }
    }

    void connect_to_server(const asio::ip::tcp::resolver::results_type& endpoints)
    {
      if (m_owner_type == owner::client)
      {
        asio::async_connect(m_socket, endpoints,
          [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
          {
            if (!ec)
            {
              read_validation();
            }
          }
        );
      }
    }

    void disconnect()
    {
      if (is_connected())
        asio::post(m_asio_context, [this]() { m_socket.close(); });
    }

    bool is_connected() const
    {
      return m_socket.is_open();
    }

    // (ASYNC) Send a message to the remote end of this connection
    void send(const message<T>& msg)
    {
      if (!m_is_ready)
        throw std::runtime_error("Connection is not ready to send messages");

      asio::post(m_asio_context,
        [this, msg]()
        {
          bool is_writing_message = !m_messages_out.empty();
          m_messages_out.push_back(msg);
          if (!is_writing_message)
          {
            write_header();
          }
        }
      );
    }

  private:
    // (ASYNC) Prime context ready to write a message
    void write_header()
    {
      if (m_messages_out.front().size() > m_remote_config.max_message_size)
        throw std::runtime_error("Message size exceeds maximum message size");

      asio::async_write(m_socket, asio::buffer(&m_messages_out.front().header, sizeof(message_header<T>)),
        [this](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            if (m_messages_out.front().body.size() > 0)
            {
              write_body();
            }
            else
            {
              m_messages_out.pop_front();

              if (!m_messages_out.empty())
              {
                write_header();
              }
            }
          }
          else
          {
            std::cout << "[" << get_id() << "] Write Header Fail.\n";
            m_socket.close();
          }
        }
      );
    }

    // (ASYNC) Prime context ready to write a message body
    void write_body()
    {
      asio::async_write(m_socket, asio::buffer(m_messages_out.front().body.data(), m_messages_out.front().body.size()),
        [this](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            m_messages_out.pop_front();

            if (!m_messages_out.empty())
            {
              write_header();
            }
          }
          else
          {
            std::cout << "[" << get_id() << "] Write Body Fail.\n";
            m_socket.close();
          }
        }
      );
    }

    // (ASYNC) Prime context ready to read a message header
    void read_header()
    {
      asio::async_read(m_socket, asio::buffer(&m_msg_temp_in.header, sizeof(message_header<T>)),
        [this](std::error_code ec, std::size_t length)
        {
          if (!ec && m_msg_temp_in.header.size <= m_owner_config.max_message_size)
          {
            if (m_msg_temp_in.header.size > 0)
            {
              m_msg_temp_in.body.resize(m_msg_temp_in.header.size);
              read_body();
            }
            else
            {
              add_to_incoming_message_queue();
            }
          }
          else
          {
            std::cout << "[" << get_id() << "] Read Header Fail.\n";
            m_socket.close();
          }
        }
      );
    }

    // (ASYNC) Prime context ready to read a message body
    void read_body()
    {
      asio::async_read(m_socket, asio::buffer(m_msg_temp_in.body.data(), m_msg_temp_in.body.size()),
        [this](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            add_to_incoming_message_queue();
          }
          else
          {
            std::cout << "[" << get_id() << "] Read Body Fail.\n";
            m_socket.close();
          }
        }
      );
    }

    // Add incoming message to queue
    void add_to_incoming_message_queue()
    {
      if (m_owner_type == owner::server)
        m_messages_in.push_back({ this->shared_from_this(), m_msg_temp_in });
      else
        m_messages_in.push_back({ nullptr, m_msg_temp_in });

      read_header();
    }

    // Naive implementation. It only protects against accidental connections.
    uint64_t scramble(uint64_t value)
    {
      uint64_t out = value ^ 0xCF2911F740C52729;
      out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
      return out ^ 0xAF1163442D8647C0;
    }

    // (ASYNC) Prime context ready to write validation
    void write_validation()
    {
      asio::async_write(m_socket, asio::buffer(&m_handshake_out, sizeof(uint64_t)),
        [this](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            if (m_owner_type == owner::client)
              read_config();
          }
          else
          {
            m_socket.close();
          }
        }
      );
    }

    // (ASYNC) Prime context ready to read validation
    void read_validation(server_interface<T>* server = nullptr)
    {
      asio::async_read(m_socket, asio::buffer(&m_handshake_in, sizeof(uint64_t)),
        [this, server](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            if (m_owner_type == owner::server)
            {
              if (m_handshake_in == m_handshake_check)
              {
                std::cout << "[" << get_id() << "] Client Validated\n";
                server->on_client_validated(this->shared_from_this());
                write_config();
                read_config(server);
              }
              else
              {
                std::cout << "[" << get_id() << "] Client Disconnected (Validation Fail)\n";
                m_socket.close();
              }
            }
            else
            {
              m_handshake_out = scramble(m_handshake_in);
              write_validation();
            }
          }
          else
          {
            std::cout << "[" << get_id() << "] Client Disconnected (read_validation)\n";
            m_socket.close();
          }
        }
      );
    }

    // (ASYNC) Prime context ready to write config
    void write_config()
    {
      asio::async_write(m_socket, asio::buffer(&m_owner_config, sizeof(config)),
        [this](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            if (m_owner_type == owner::client)
            {
              m_is_ready = true;
              read_header();
            }
          }
          else
          {
            std::cout << "[" << get_id() << "] Write Config Fail.\n";
            m_socket.close();
          }
        }
      );
    }

    // (ASYNC) Prime context ready to read config
    void read_config(server_interface<T>* server = nullptr)
    {
      asio::async_read(m_socket, asio::buffer(&m_remote_config, sizeof(config)),
        [this, server](std::error_code ec, std::size_t length)
        {
          if (!ec)
          {
            // Check config
            if (m_remote_config.endian == m_owner_config.endian && m_remote_config.version == m_owner_config.version)
            {
              if (m_owner_type == owner::server)
              {
                std::cout << "[" << get_id() << "] Client Config Validated\n";
                server->on_client_config_validated(this->shared_from_this());
                m_is_ready = true;
                server->on_client_ready(this->shared_from_this());
                read_header();
              }
              else
                write_config();
            }
            else
            {
              if (m_owner_type == owner::server)
                std::cout << "[" << get_id() << "] Client Disconnected (Config Fail)\n";
              else
                std::cout << "[" << get_id() << "] Server Disconnected (Config Fail)\n";
              m_socket.close();
            }
          }
          else
          {
            std::cout << "[" << get_id() << "] Read Config Fail.\n";
            m_socket.close();
          }
        }
      );
    }

  protected:
    // Each connection has a unique socket to a remote
    asio::ip::tcp::socket m_socket;

    // This context is shared with the whole asio instance
    asio::io_context& m_asio_context;

    // This queue holds all messages to be sent to the remote side of this connection
    tsqueue<message<T>> m_messages_out;

    // This queue holds all messages that have been received from the remote side of this connection
    tsqueue<owned_message<T>>& m_messages_in;
    message<T> m_msg_temp_in;

    // Owner of this connection
    owner m_owner_type = owner::server;

    // Identifier of client
    uint32_t m_id = 0;

    // Handshake validation
    uint64_t m_handshake_out = 0;
    uint64_t m_handshake_in = 0;
    uint64_t m_handshake_check = 0;

    // Client's and server's configuration
    config_view m_owner_config;
    config m_remote_config;

    // Is connection ready of message exchange
    bool m_is_ready = false;
  };

}
