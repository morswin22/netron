#include "common.hpp"

#ifdef OS_OTHER
  #include <future>
#endif

class CustomClient : public netron::client_interface<CustomMessageTypes>
{
public:
  void PingServer()
  {
    Message msg;
    msg.header.id = CustomMessageTypes::ServerPing;

    std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
    msg << time_now;
    
    send(msg);
  }

  void MessageAll()
  {
    Message msg;
    msg.header.id = CustomMessageTypes::MessageAll;
    send(msg); // message without body
  }

  void SendString()
  {
    Message msg;
    msg.header.id = CustomMessageTypes::SendComplexData;
    msg << std::string("Hello from client!");
    msg << std::vector<Point>{ Point(1, 9), Point(1, 0) };
    msg << std::list<std::vector<Point>>{ { Point(1, 2), Point(3, 4) }, { Point(5, 6), Point(7, 8) } };
    send(msg);
  }
};

int main(void)
{
  CustomClient client;
  client.connect("127.0.0.1", 60000);

#ifdef OS_WINDOWS
  bool key[4] = { false, false, false, false };
  bool old_key[4] = { false, false, false, false };

  std::cout << "Press:\n";
  std::cout << "1: Ping Server\n";
  std::cout << "2: Message All\n";
  std::cout << "3: Send complex data\n";
  std::cout << "4: Exit\n";
#else
  auto job = std::async(std::launch::async, 
    [&client]()
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      client.PingServer();
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      client.MessageAll();
    }
  );
#endif

  bool should_quit = false;
  while (!should_quit)
  {
#ifdef OS_WINDOWS
    if (GetForegroundWindow() == GetConsoleWindow())
    {
      key[0] = GetAsyncKeyState('1') & 0x8000;
      key[1] = GetAsyncKeyState('2') & 0x8000;
      key[2] = GetAsyncKeyState('3') & 0x8000;
      key[3] = GetAsyncKeyState('4') & 0x8000;
    }

    if (key[0] && !old_key[0]) 
      client.PingServer();

    if (key[1] && !old_key[1])
      client.MessageAll();

    if (key[2] && !old_key[2])
      client.SendString();

    if (key[3] && !old_key[3]) 
      should_quit = true;

    for (int i = 0; i < 4; i++) 
      old_key[i] = key[i];
#endif

    if (client.is_connected())
    {
      if (!client.incoming().empty())
      {
        auto msg = client.incoming().pop_front().msg;

        switch (msg.header.id)
        {
        case CustomMessageTypes::ServerAccept:
        {
          std::cout << "Server Accepted Connection\n";
        }
        break;

        case CustomMessageTypes::ServerPing:
        {
          std::chrono::system_clock::time_point time_now = std::chrono::system_clock::now();
          std::chrono::system_clock::time_point time_then;
          msg >> time_then;
          std::cout << "Ping: " << std::chrono::duration<double>(time_now - time_then).count() << "s\n";
        }
        break;

        case CustomMessageTypes::ServerMessage:
        {
          uint32_t clientID;
          msg >> clientID;
          std::cout << "Hello from [" << clientID << "]\n";
        }
        break;
        }
      }
    }
    else
    {
      std::cout << "Server Down\n";
      should_quit = true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  return 0;
}
