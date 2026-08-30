#pragma once

#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/routing/arp_cache.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"

#include <variant>

namespace silicon_switch::routing {

enum class L3EncapsulationFailure {
    invalid_source_mac,
    neighbor_not_found,
    packet_too_large,
};

using L3EncapsulationResult =
    std::variant<network::EthernetFrame, L3EncapsulationFailure>;

[[nodiscard]] L3EncapsulationResult encapsulate_ipv4_in_ethernet(
    const ForwardedIpv4Packet& forwarding,
    const network::MacAddress& source_mac,
    const ArpCache& arp_cache);

[[nodiscard]] L3EncapsulationResult encapsulate_ipv4_in_ethernet(
    const ForwardedIpv4Packet& forwarding,
    const network::MacAddress& source_mac,
    const ArpCache& arp_cache,
    ArpCache::TimePoint now);

}  // namespace silicon_switch::routing
