#pragma once

#include <netron/common.hpp>
#include <netron/asio.hpp>
#include <netron/tsqueue.hpp>
#include <netron/message.hpp>
#include <netron/connection.hpp>

namespace netron 
{

  template<typename T>
  class server_interface
  {
  public:
    using Client = std::shared_ptr<connection<T>>;
    using Message = message<T>;

    server_interface(uint16_t port)
      : m_asio_acceptor(m_asio_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {

    }

    virtual ~server_interface()
    {
      stop();
    }

    bool start()
    {
      try
      {
        wait_for_client_connection();

        m_thread_context = std::thread([this]() { m_asio_context.run(); });
      }
      catch(std::exception& e)
      {
        std::cerr << "Server Exception: " << e.what() << '\n';
        return false;
      }

      std::cout << "Server Started!\n";
      return true;
    }

    void stop()
    {
      // Request the context to close
      m_asio_context.stop();

      // Tidy up the context thread
      if (m_thread_context.joinable())
        m_thread_context.join();

      std::cout << "Server Stopped!\n";
    }

    // (ASYNC) Instruct asio to wait for a connection
    void wait_for_client_connection()
    {
      m_asio_acceptor.async_accept(
        [this](std::error_code ec, asio::ip::tcp::socket socket)
        {
          if (!ec)
          {
            std::cout << "New Connection: " << socket.remote_endpoint() << '\n';

            auto new_connection = std::make_shared<connection<T>>(connection<T>::owner::server, m_asio_context, std::move(socket), m_messages_in);
          
            if (on_client_connect(new_connection))
            {
              m_connections.push_back(std::move(new_connection));
              m_connections.back()->connect_to_client(this, m_id_counter++);
              std::cout << "[" << m_connections.back()->get_id() << "] Connection Approved" << '\n';
            }
            else
            {
              std::cout << "Rejected Connection.\n";
            }
          }
          else
          {
            std::cerr << "New Connection Error: " << ec.message() << '\n';
          }

          // Wait for another connection
          wait_for_client_connection();
        }
      );
    }

    // Send a message to a specific client
    void message_client(Client client, const Message& msg)
    {
      if (client && client->is_connected())
      {
        client->send(msg);
      }
      else
      {
        on_client_disconnect(client);
        client.reset();
        m_connections.erase(
          std::remove(m_connections.begin(), m_connections.end(), client), m_connections.end()
        );
      }
    }

    // Send a message to all clients
    void message_all_clients(const Message& msg, Client ignore_client = nullptr)
    {
      bool does_invalid_client_exist = false;
      for (auto& client : m_connections)
      {
        if (client && client->is_connected())
        {
          if (client != ignore_client)
            client->send(msg);
        }
        else
        {
          on_client_disconnect(client);
          client.reset();
          does_invalid_client_exist = true;
        }
      }

      if (does_invalid_client_exist)
        m_connections.erase(
          std::remove(m_connections.begin(), m_connections.end(), nullptr), m_connections.end()
        );
    }

    void update(size_t max_messages = std::numeric_limits<size_t>::max(), bool wait = false)
    {
      if (wait)
        m_messages_in.wait();

      size_t message_count = 0;
      while (message_count < max_messages && !m_messages_in.empty())
      {
        // Grab the front message
        auto msg = m_messages_in.pop_front();

        // Pass to message handler
        on_message(msg.remote, msg.msg);

        message_count++;
      }
    }

  protected:
    // Called when a client connects, has an option to reject the connection
    virtual bool on_client_connect(Client client)
    {
      return false;
    }

    // Called when a client appears to have disconnected
    virtual void on_client_disconnect(Client client)
    {

    }

    // Called when a message arrives
    virtual void on_message(Client client, Message& msg)
    {

    }

  public:
    // Called when a client has been validated
    virtual void on_client_validated(Client client)
    {

    }

  protected:
    // incoming messages from connected clients
    tsqueue<owned_message<T>> m_messages_in;

    // Container of active validated connections
    std::deque<Client> m_connections;

    // Asio context handles the data transfer
    asio::io_context m_asio_context;

    // Thread for asio context
    std::thread m_thread_context;

    // Asio acceptor
    asio::ip::tcp::acceptor m_asio_acceptor;

    // Unique identifier for a client
    uint32_t m_id_counter = 10000;
  };

}
