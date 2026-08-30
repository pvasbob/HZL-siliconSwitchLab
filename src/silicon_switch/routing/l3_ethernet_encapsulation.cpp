#include "silicon_switch/routing/l3_ethernet_encapsulation.hpp"

#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/routing/arp_cache.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"

#include <utility>

namespace silicon_switch::routing {
namespace {

[[nodiscard]] bool is_valid_source_mac(
    const network::MacAddress& address) noexcept {
    constexpr network::MacAddress::Bytes unspecified_bytes{};
    return address != network::MacAddress{unspecified_bytes} &&
           address.is_unicast();
}

[[nodiscard]] L3EncapsulationResult encapsulate_with_destination(
    const ForwardedIpv4Packet& forwarding,
    const network::MacAddress& source_mac,
    const network::MacAddress& destination_mac) {
    auto payload = forwarding.packet().serialize();
    if (payload.size() > network::EthernetFrame::maximum_payload_size) {
        return L3EncapsulationFailure::packet_too_large;
    }

    auto frame = network::EthernetFrame::create(
        destination_mac,
        source_mac,
        network::EtherType::ipv4,
        std::move(payload));
    if (!frame.has_value()) {
        return L3EncapsulationFailure::packet_too_large;
    }

    return std::move(*frame);
}

}  // namespace

L3EncapsulationResult encapsulate_ipv4_in_ethernet(
    const ForwardedIpv4Packet& forwarding,
    const network::MacAddress& source_mac,
    const ArpCache& arp_cache) {
    return encapsulate_ipv4_in_ethernet(
        forwarding, source_mac, arp_cache, ArpCache::Clock::now());
}

L3EncapsulationResult encapsulate_ipv4_in_ethernet(
    const ForwardedIpv4Packet& forwarding,
    const network::MacAddress& source_mac,
    const ArpCache& arp_cache,
    const ArpCache::TimePoint now) {
    if (!is_valid_source_mac(source_mac)) {
        return L3EncapsulationFailure::invalid_source_mac;
    }

    const auto destination_mac = arp_cache.lookup(forwarding.next_hop(), now);
    if (!destination_mac.has_value()) {
        return L3EncapsulationFailure::neighbor_not_found;
    }

    return encapsulate_with_destination(
        forwarding, source_mac, *destination_mac);
}

}  // namespace silicon_switch::routing
