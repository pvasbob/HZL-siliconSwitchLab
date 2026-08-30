#pragma once

#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"
#include "silicon_switch/routing/ipv4_route_table.hpp"

namespace silicon_switch::routing {

class Ipv4ForwardingEngine {
public:
    explicit Ipv4ForwardingEngine(Ipv4RouteTable route_table);

    [[nodiscard]] Ipv4ForwardingResult forward(
        const network::Ipv4Packet& packet) const;

    [[nodiscard]] const Ipv4RouteTable& route_table() const noexcept {
        return route_table_;
    }

private:
    Ipv4RouteTable route_table_;
};

}  // namespace silicon_switch::routing
