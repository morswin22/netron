#pragma once

#include <netron/common.hpp>
#include <netron/asio.hpp>
#include <netron/tsqueue.hpp>
#include <netron/message.hpp>
#include <netron/connection.hpp>

namespace netron 
{

  template<typename T>
  class client_interface
  {
  public:
    using Message = message<T>;

    client_interface()
    {}

    virtual ~client_interface()
    {
      disconnect();
    }
  public:

    // Connect to server with hostname/ip-address and port
    bool connect(const std::string& host, const uint16_t port)
    {
      try
      {
        // Resolve hostname/ip-address into tangable physical address
        asio::ip::tcp::resolver resolver(m_asio_context);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        // Create connection
        m_connection = std::make_unique<connection<T>>(
          connection<T>::owner::client,
          m_asio_context,
          asio::ip::tcp::socket(m_asio_context),
          m_messages_in
        );

        // Tell the connection object to connect to server
        m_connection->connect_to_server(endpoints);

        // Start the context thread
        m_thread_context = std::thread([this]() { m_asio_context.run(); });
      }
      catch(std::exception& e)
      {
        std::cerr << "Client Exception: " << e.what() << '\n';
        return false;
      }
      
      return false;
    }

    // Disconnect from server
    void disconnect()
    {
      if (is_connected())
        m_connection->disconnect();

      m_asio_context.stop();
      if (m_thread_context.joinable())
        m_thread_context.join();

      m_connection.release();
    }

    // Check if client is connected to server
    bool is_connected() const
    {
      if (m_connection)
        return m_connection->is_connected();
      else
        return false;
    }

    // Send message to server
    void send(const Message& msg)
    {
      if (is_connected())
        m_connection->send(msg);
    }

    // Retrieve queue of incoming messages
    tsqueue<owned_message<T>>& incoming()
    {
      return m_messages_in;
    }

  protected:
    // Asio context handles the data transfer
    asio::io_context m_asio_context;

    // Thread of execution for the asio context
    std::thread m_thread_context;

    // A single instance of a connection object to server
    std::unique_ptr<connection<T>> m_connection;

  private:
    // This queue holds all incoming messages from the server
    tsqueue<owned_message<T>> m_messages_in;
  };

}
