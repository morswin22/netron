#include "common.hpp"

class CustomServer : public netron::server_interface<CustomMessageTypes>
{
public:
  CustomServer(uint16_t port)
    : netron::server_interface<CustomMessageTypes>(port)
  {
  }

protected:
  virtual bool on_client_connect(Client client)
  {
    Message msg;
    msg.header.id = CustomMessageTypes::ServerAccept;
    client->send(msg); // Send a welcome message

    return true; // Accept all new connections
  }

  virtual void on_client_disconnect(Client client)
  {
    std::cout << "Removing client [" << client->get_id() << "]\n";
  }

  virtual void on_message(Client client, Message& msg)
  {
    switch (msg.header.id)
    {
    case CustomMessageTypes::ServerPing:
    {
      std::cout << "[" << client->get_id() << "]: Server Ping\n";
      client->send(msg); // bounce message back to client
    }
    break;

    case CustomMessageTypes::MessageAll:
    {
      std::cout << "[" << client->get_id() << "]: Message All\n";
      Message msg;
      msg.header.id = CustomMessageTypes::ServerMessage;
      msg << client->get_id();
      message_all_clients(msg, client); // send message to all connected clients except this one
    }
    break;
    }
  }
};

int main(void)
{
  CustomServer server(60000);
  server.start();

  while (true)
  {
    server.update();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return 0;
}
