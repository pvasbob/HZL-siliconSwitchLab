#include "silicon_switch/routing/ipv4_forwarding_engine.hpp"

#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"
#include "silicon_switch/routing/ipv4_ttl.hpp"

#include <utility>
#include <variant>

namespace silicon_switch::routing {

Ipv4ForwardingEngine::Ipv4ForwardingEngine(Ipv4RouteTable route_table)
    : route_table_{std::move(route_table)} {}

Ipv4ForwardingResult Ipv4ForwardingEngine::forward(
    const network::Ipv4Packet& packet) const {
    auto ttl_result = decrement_ipv4_ttl(packet);
    if (std::holds_alternative<DroppedIpv4Packet>(ttl_result)) {
        return std::get<DroppedIpv4Packet>(ttl_result);
    }

    const auto route = route_table_.longest_prefix_match(packet.destination());
    if (!route.has_value()) {
        return DroppedIpv4Packet{Ipv4DropReason::route_not_found};
    }

    const network::Ipv4Address next_hop =
        route->next_hop().value_or(packet.destination());
    return ForwardedIpv4Packet{
        std::get<network::Ipv4Packet>(std::move(ttl_result)),
        route->output_port(),
        next_hop};
}

}  // namespace silicon_switch::routing
