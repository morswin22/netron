#pragma once
#include <cstdint>

namespace netron
{

  inline namespace literals 
  {

    using byte_size = uint64_t;

    constexpr byte_size operator "" _B(size_t bytes)
    {
      return static_cast<byte_size>(bytes);
    }

    constexpr byte_size operator "" _KB(size_t kilobytes)
    {
      return static_cast<byte_size>(kilobytes) * 1024_B;
    }

    constexpr byte_size operator "" _KB(long double kilobytes)
    {
      return static_cast<byte_size>(kilobytes * 1024_B);
    }

    constexpr byte_size operator "" _MB(size_t megabytes)
    {
      return static_cast<byte_size>(megabytes) * 1024_KB;
    }

    constexpr byte_size operator "" _MB(long double megabytes)
    {
      return static_cast<byte_size>(megabytes * 1024_KB);
    }

    constexpr byte_size operator "" _GB(size_t gigabytes)
    {
      return static_cast<byte_size>(gigabytes) * 1024_MB;
    }

    constexpr byte_size operator "" _GB(long double gigabytes)
    {
      return static_cast<byte_size>(gigabytes * 1024_MB);
    }

    constexpr byte_size operator "" _TB(size_t terabytes)
    {
      return static_cast<byte_size>(terabytes) * 1024_GB;
    }

    constexpr byte_size operator "" _TB(long double terabytes)
    {
      return static_cast<byte_size>(terabytes * 1024_GB);
    }

  }

}