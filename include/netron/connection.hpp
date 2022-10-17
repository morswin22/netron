#pragma once

#include <netron/common.hpp>
#include <netron/asio.hpp>
#include <netron/tsqueue.hpp>
#include <netron/message.hpp>

namespace netron 
{

  template<typename T>
  class connection : public std::enable_shared_from_this<connection<T>>
  {
  public:
    enum class owner
    {
      server,
      client
    };

    connection(owner parent, asio::io_context& asio_context, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& messages_in)
      : m_asio_context(asio_context), m_socket(std::move(socket)), m_messages_in(messages_in)
    {
      m_owner_type = parent;
    }

    virtual ~connection()
    {}

    uint32_t get_id() const
    {
      return m_id;
    }

    void connect_to_client(uint32_t uid = 0)
    {
      if (m_owner_type == owner::server)
      {
        if (m_socket.is_open())
        {
          m_id = uid;
          read_header();
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
              read_header();
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

    // TODO [unused]
    void start_listening()
    {

    }

    // (ASYNC) Send a message to the remote end of this connection
    void send(const message<T>& msg)
    {
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
            std::cout << "[" << m_id << "] Write Header Fail.\n";
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
            std::cout << "[" << m_id << "] Write Body Fail.\n";
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
          if (!ec)
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
            std::cout << "[" << m_id << "] Read Header Fail.\n";
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
            std::cout << "[" << m_id << "] Read Body Fail.\n";
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
  };

}
