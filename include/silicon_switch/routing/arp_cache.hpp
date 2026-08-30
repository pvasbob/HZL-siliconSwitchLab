#pragma once

#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/mac_address.hpp"

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>

namespace silicon_switch::routing {

enum class ArpCacheUpdate {
    inserted,
    replaced,
    rejected,
};

class ArpCache {
public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;
    using TimePoint = Clock::time_point;

    inline static constexpr std::chrono::seconds default_entry_lifetime{300};

    explicit ArpCache(Duration entry_lifetime = default_entry_lifetime);

    [[nodiscard]] ArpCacheUpdate add_or_replace(
        network::Ipv4Address address,
        network::MacAddress mac_address);

    [[nodiscard]] ArpCacheUpdate add_or_replace(
        network::Ipv4Address address,
        network::MacAddress mac_address,
        TimePoint learned_at);

    [[nodiscard]] std::optional<network::MacAddress> lookup(
        network::Ipv4Address address) const;

    [[nodiscard]] std::optional<network::MacAddress> lookup(
        network::Ipv4Address address,
        TimePoint now) const;

    [[nodiscard]] bool remove(network::Ipv4Address address);

    [[nodiscard]] std::size_t expire(TimePoint now);

    [[nodiscard]] constexpr Duration entry_lifetime() const noexcept {
        return entry_lifetime_;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return entries_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return entries_.empty();
    }

private:
    struct Entry {
        network::MacAddress mac_address;
        TimePoint learned_at;
    };

    [[nodiscard]] bool is_expired(const Entry& entry, TimePoint now) const;

    Duration entry_lifetime_;
    std::map<network::Ipv4Address, Entry> entries_;
};

}  // namespace silicon_switch::routing
