#include "silicon_switch/routing/ipv4_route_table.hpp"

#include <algorithm>
#include <optional>
#include <utility>

namespace silicon_switch::routing {

RouteUpdate Ipv4RouteTable::add_or_replace(RouteEntry entry) {
    const auto existing = std::find_if(
        entries_.begin(),
        entries_.end(),
        [&entry](const RouteEntry& candidate) {
            return candidate.prefix() == entry.prefix();
        });

    if (existing != entries_.end()) {
        *existing = std::move(entry);
        return RouteUpdate::replaced;
    }

    entries_.push_back(std::move(entry));
    return RouteUpdate::inserted;
}

bool Ipv4RouteTable::remove(const network::Ipv4Prefix& prefix) {
    const auto existing = std::find_if(
        entries_.begin(),
        entries_.end(),
        [&prefix](const RouteEntry& entry) {
            return entry.prefix() == prefix;
        });
    if (existing == entries_.end()) {
        return false;
    }

    entries_.erase(existing);
    return true;
}

std::optional<RouteEntry> Ipv4RouteTable::find_exact(
    const network::Ipv4Prefix& prefix) const {
    const auto existing = std::find_if(
        entries_.begin(),
        entries_.end(),
        [&prefix](const RouteEntry& entry) {
            return entry.prefix() == prefix;
        });
    if (existing == entries_.end()) {
        return std::nullopt;
    }

    return *existing;
}

std::optional<RouteEntry> Ipv4RouteTable::longest_prefix_match(
    const network::Ipv4Address destination) const {
    const RouteEntry* best_match = nullptr;

    for (const RouteEntry& entry : entries_) {
        if (entry.prefix().contains(destination) &&
            (best_match == nullptr ||
             entry.prefix().length() > best_match->prefix().length())) {
            best_match = &entry;
        }
    }

    if (best_match == nullptr) {
        return std::nullopt;
    }

    return *best_match;
}

}  // namespace silicon_switch::routing
