#pragma once

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "silicon_switch/routing/port_id.hpp"

#include <optional>

namespace silicon_switch::routing {

class RouteEntry {
public:
    [[nodiscard]] static constexpr std::optional<RouteEntry> create(
        const network::Ipv4Prefix prefix,
        const std::optional<network::Ipv4Address> next_hop,
        const PortId output_port) noexcept {
        if (next_hop.has_value() &&
            (next_hop->is_unspecified() || next_hop->is_multicast() ||
             next_hop->is_limited_broadcast())) {
            return std::nullopt;
        }

        return RouteEntry{prefix, next_hop, output_port, ValidatedTag{}};
    }

    [[nodiscard]] constexpr network::Ipv4Prefix prefix() const noexcept {
        return prefix_;
    }

    [[nodiscard]] constexpr std::optional<network::Ipv4Address>
    next_hop() const noexcept {
        return next_hop_;
    }

    [[nodiscard]] constexpr PortId output_port() const noexcept {
        return output_port_;
    }

    [[nodiscard]] constexpr bool is_directly_connected() const noexcept {
        return !next_hop_.has_value();
    }

    bool operator==(const RouteEntry&) const noexcept = default;

private:
    struct ValidatedTag {};

    explicit constexpr RouteEntry(
        const network::Ipv4Prefix prefix,
        const std::optional<network::Ipv4Address> next_hop,
        const PortId output_port,
        ValidatedTag) noexcept
        : prefix_{prefix},
          next_hop_{next_hop},
          output_port_{output_port} {}

    network::Ipv4Prefix prefix_;
    std::optional<network::Ipv4Address> next_hop_;
    PortId output_port_;
};

}  // namespace silicon_switch::routing
