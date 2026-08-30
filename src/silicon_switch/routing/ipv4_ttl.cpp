#include "silicon_switch/routing/ipv4_ttl.hpp"

#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"

#include <cstdint>
#include <utility>

namespace silicon_switch::routing {

Ipv4TtlUpdateResult decrement_ipv4_ttl(
    const network::Ipv4Packet& packet) {
    if (packet.time_to_live() <= 1U) {
        return DroppedIpv4Packet{Ipv4DropReason::time_to_live_expired};
    }

    const auto decremented_packet = network::Ipv4Packet::create(
        packet.source(),
        packet.destination(),
        packet.protocol(),
        packet.payload(),
        static_cast<std::uint8_t>(packet.time_to_live() - 1U),
        packet.identification(),
        packet.dont_fragment());

    return std::move(*decremented_packet);
}

}  // namespace silicon_switch::routing
