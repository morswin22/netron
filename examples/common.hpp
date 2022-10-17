#pragma once
#include <netron.hpp>

#ifdef _WIN32
  #define OS_WINDOWS
#else
  #define OS_OTHER
#endif

enum class CustomMessageTypes : uint32_t
{
  ServerAccept,
  ServerDeny,
  ServerPing,
  MessageAll,
  ServerMessage,
};
