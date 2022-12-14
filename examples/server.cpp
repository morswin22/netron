#include "common.hpp"

class CustomServer : public netron::server_interface<CustomMessageTypes>
{
public:
  CustomServer(uint16_t port)
    : netron::server_interface<CustomMessageTypes>(port)
  {
  }

protected:
  virtual void on_client_ready(Client client)
  {
    Message msg;
    msg.header.id = CustomMessageTypes::ServerAccept;
    client->send(msg); // Send a welcome message
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

    case CustomMessageTypes::SendComplexData:
    {
      std::list<std::vector<Point>> list;
      std::vector<Point> vec;
      std::string str;
      msg >> list >> vec >> str;

      std::cout << "[" << client->get_id() << "]: Sent string '" << str << "'\n";

      std::cout << "[" << client->get_id() << "]: and vector of size " << vec.size() << " -> ";
      for (auto& i : vec)
        std::cout << i << " ";
      std::cout << "\n";

      std::cout << "[" << client->get_id() << "]: and list of size " << list.size() << " -> ";
      for (auto& i : list)
      {
        for (auto& j : i)
          std::cout << j << " ";
        std::cout << " | ";
      }
      std::cout << "\n";
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
    server.update(std::numeric_limits<size_t>::max(), true);
  }

  return 0;
}
