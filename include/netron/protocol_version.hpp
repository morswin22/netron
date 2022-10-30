#pragma once
#include <iostream>

namespace netron
{

  struct protocol_version
  {
    uint8_t major = 0;
    uint8_t minor = 0;
  };

  constexpr bool operator==(const protocol_version& lhs, const protocol_version& rhs)
  {
    return lhs.major == rhs.major && lhs.minor == rhs.minor;
  }

  constexpr bool operator!=(const protocol_version& lhs, const protocol_version& rhs)
  {
    return lhs.major != rhs.major || lhs.minor != rhs.minor;
  }

  constexpr bool operator<(const protocol_version& lhs, const protocol_version& rhs)
  {
    return lhs.major < rhs.major || (lhs.major == rhs.major && lhs.minor < rhs.minor);
  }

  constexpr bool operator>(const protocol_version& lhs, const protocol_version& rhs)
  {
    return lhs.major > rhs.major || (lhs.major == rhs.major && lhs.minor > rhs.minor);
  }

  constexpr bool operator<=(const protocol_version& lhs, const protocol_version& rhs)
  {
    return lhs.major < rhs.major || (lhs.major == rhs.major && lhs.minor <= rhs.minor);
  }

  constexpr bool operator>=(const protocol_version& lhs, const protocol_version& rhs)
  {
    return lhs.major > rhs.major || (lhs.major == rhs.major && lhs.minor >= rhs.minor);
  }

  std::ostream& operator<<(std::ostream& os, const protocol_version& version)
  {
    os << int(version.major) << "." << int(version.minor);
    return os;
  }

  inline namespace literals 
  {

    constexpr protocol_version operator "" _pv(const char* version, size_t len)
    {
      protocol_version pv;
      bool dot = false;
      for (auto i = 0; i < len; ++i)
      {
        const auto c = version[i];
        if (c == '.')
        {
          dot = true;
          continue;
        }
        if (dot)
        {
          pv.minor *= 10;
          pv.minor += version[i] - '0';
        }
        else
        {
          pv.major *= 10;
          pv.major += version[i] - '0';
        }
      }
      return pv;
    }

  }

}