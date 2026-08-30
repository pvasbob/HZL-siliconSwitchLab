#pragma once

#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"

#include <variant>

namespace silicon_switch::routing {

using Ipv4TtlUpdateResult =
    std::variant<network::Ipv4Packet, DroppedIpv4Packet>;

[[nodiscard]] Ipv4TtlUpdateResult decrement_ipv4_ttl(
    const network::Ipv4Packet& packet);

}  // namespace silicon_switch::routing
