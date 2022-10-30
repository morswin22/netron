#pragma once

namespace netron
{

  enum class endian : uint8_t
  {
    little = 0,
    big    = 1,
#ifdef _WIN32
    native = little
#else
  #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    native = little
  #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    native = big
  #else
    #error "Unknown endianness!"
  #endif
#endif
  };

}