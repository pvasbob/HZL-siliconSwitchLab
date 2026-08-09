#pragma once

#include <cstdint>

namespace silicon_switch::network {

enum class IpProtocol : std::uint8_t {
    icmp = 1U,
    tcp = 6U,
    udp = 17U,
};

}  // namespace silicon_switch::network
