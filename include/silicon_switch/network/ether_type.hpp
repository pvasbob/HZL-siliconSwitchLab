#pragma once

#include <cstdint>

namespace silicon_switch::network {

enum class EtherType : std::uint16_t {
    ipv4 = 0x0800U,
    arp = 0x0806U,
    vlan_tagged = 0x8100U,
    ipv6 = 0x86DDU,
};

}  // namespace silicon_switch::network
