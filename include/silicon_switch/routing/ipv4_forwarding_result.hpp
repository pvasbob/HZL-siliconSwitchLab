#pragma once

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/port_id.hpp"

#include <utility>
#include <variant>

namespace silicon_switch::routing {

enum class Ipv4DropReason {
    time_to_live_expired,
    route_not_found,
};

class ForwardedIpv4Packet {
public:
    ForwardedIpv4Packet(
        network::Ipv4Packet packet,
        const PortId output_port,
        const network::Ipv4Address next_hop)
        : packet_{std::move(packet)},
          output_port_{output_port},
          next_hop_{next_hop} {}

    [[nodiscard]] const network::Ipv4Packet& packet() const noexcept {
        return packet_;
    }

    [[nodiscard]] constexpr PortId output_port() const noexcept {
        return output_port_;
    }

    [[nodiscard]] constexpr network::Ipv4Address next_hop() const noexcept {
        return next_hop_;
    }

    [[nodiscard]] bool operator==(
        const ForwardedIpv4Packet& other) const noexcept {
        return packet_ == other.packet_ && output_port_ == other.output_port_ &&
               next_hop_ == other.next_hop_;
    }

private:
    network::Ipv4Packet packet_;
    PortId output_port_;
    network::Ipv4Address next_hop_;
};

class DroppedIpv4Packet {
public:
    explicit constexpr DroppedIpv4Packet(
        const Ipv4DropReason reason) noexcept
        : reason_{reason} {}

    [[nodiscard]] constexpr Ipv4DropReason reason() const noexcept {
        return reason_;
    }

    [[nodiscard]] constexpr bool operator==(
        const DroppedIpv4Packet& other) const noexcept {
        return reason_ == other.reason_;
    }

private:
    Ipv4DropReason reason_;
};

using Ipv4ForwardingResult =
    std::variant<ForwardedIpv4Packet, DroppedIpv4Packet>;

}  // namespace silicon_switch::routing
