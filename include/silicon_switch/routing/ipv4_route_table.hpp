#pragma once

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "silicon_switch/routing/route_entry.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace silicon_switch::routing {

enum class RouteUpdate {
    inserted,
    replaced,
};

class Ipv4RouteTable {
public:
    [[nodiscard]] RouteUpdate add_or_replace(RouteEntry entry);

    [[nodiscard]] bool remove(const network::Ipv4Prefix& prefix);

    [[nodiscard]] std::optional<RouteEntry>
    find_exact(const network::Ipv4Prefix& prefix) const;

    [[nodiscard]] std::optional<RouteEntry>
    longest_prefix_match(network::Ipv4Address destination) const;

    [[nodiscard]] std::size_t size() const noexcept {
        return entries_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return entries_.empty();
    }

private:
    std::vector<RouteEntry> entries_;
};

}  // namespace silicon_switch::routing
