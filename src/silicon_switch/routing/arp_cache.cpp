#include "silicon_switch/routing/arp_cache.hpp"

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/mac_address.hpp"

#include <chrono>
#include <optional>
#include <stdexcept>
#include <utility>

namespace silicon_switch::routing {
namespace {

[[nodiscard]] bool is_valid_address(
    const network::Ipv4Address address) noexcept {
    return !address.is_unspecified() && !address.is_multicast() &&
           !address.is_limited_broadcast();
}

[[nodiscard]] bool is_valid_mac_address(
    const network::MacAddress& address) noexcept {
    constexpr network::MacAddress::Bytes unspecified_bytes{};
    return address != network::MacAddress{unspecified_bytes} &&
           address.is_unicast();
}

}  // namespace

ArpCache::ArpCache(const Duration entry_lifetime)
    : entry_lifetime_{entry_lifetime} {
    if (entry_lifetime_ <= Duration::zero()) {
        throw std::invalid_argument{"ARP cache lifetime must be positive"};
    }
}

ArpCacheUpdate ArpCache::add_or_replace(
    const network::Ipv4Address address,
    network::MacAddress mac_address) {
    return add_or_replace(address, std::move(mac_address), Clock::now());
}

ArpCacheUpdate ArpCache::add_or_replace(
    const network::Ipv4Address address,
    network::MacAddress mac_address,
    const TimePoint learned_at) {
    if (!is_valid_address(address) || !is_valid_mac_address(mac_address)) {
        return ArpCacheUpdate::rejected;
    }

    const auto existing = entries_.find(address);
    if (existing == entries_.end()) {
        entries_.emplace(
            address, Entry{std::move(mac_address), learned_at});
        return ArpCacheUpdate::inserted;
    }

    existing->second = Entry{std::move(mac_address), learned_at};
    return ArpCacheUpdate::replaced;
}

std::optional<network::MacAddress> ArpCache::lookup(
    const network::Ipv4Address address) const {
    return lookup(address, Clock::now());
}

std::optional<network::MacAddress> ArpCache::lookup(
    const network::Ipv4Address address,
    const TimePoint now) const {
    const auto entry = entries_.find(address);
    if (entry == entries_.end() || is_expired(entry->second, now)) {
        return std::nullopt;
    }

    return entry->second.mac_address;
}

bool ArpCache::remove(const network::Ipv4Address address) {
    return entries_.erase(address) != 0U;
}

std::size_t ArpCache::expire(const TimePoint now) {
    std::size_t removed = 0U;
    for (auto entry = entries_.begin(); entry != entries_.end();) {
        if (is_expired(entry->second, now)) {
            entry = entries_.erase(entry);
            ++removed;
        } else {
            ++entry;
        }
    }
    return removed;
}

bool ArpCache::is_expired(
    const Entry& entry,
    const TimePoint now) const {
    return now >= entry.learned_at &&
           now - entry.learned_at >= entry_lifetime_;
}

}  // namespace silicon_switch::routing
