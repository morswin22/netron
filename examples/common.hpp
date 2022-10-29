#pragma once
#include <netron.hpp>

#ifdef _WIN32
  #define OS_WINDOWS
#else
  #define OS_OTHER
#endif

#ifdef _MSC_VER
  #pragma warning( disable: 4267 )
#endif

enum class CustomMessageTypes : uint32_t
{
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage,
  SendComplexData,
};

struct Point
{
  int x, y;

  Point() : Point(0, 0) {};
  Point(int x, int y) : x(x), y(y) {}

  friend std::ostream& operator<<(std::ostream& os, const Point& p)
  {
    os << "(" << p.x << ", " << p.y << ")";
    return os;
  }
};
