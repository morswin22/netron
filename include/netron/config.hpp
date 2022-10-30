#pragma once

#include <netron/common.hpp>
#include <netron/asio.hpp>
#include <netron/endian.hpp>
#include <netron/protocol_version.hpp>
#include <netron/size_literals.hpp>

namespace netron
{

#pragma pack(push, 1)
  struct config
  {
    endian endian = endian::native;
    protocol_version version = "1.0"_pv;
    uint32_t max_connections = std::numeric_limits<uint32_t>::max();
    byte_size max_message_size = 10_MB;
  };
#pragma pack(pop)

  using config_view = const config&;

}
