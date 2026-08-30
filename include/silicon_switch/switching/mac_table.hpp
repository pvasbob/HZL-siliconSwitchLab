#pragma once

#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"

#include <chrono>
#include <cstddef>
#include <map>
#include <optional>

namespace silicon_switch::switching {

enum class MacEntryType {
    dynamic,
    static_entry,
};

enum class MacTableUpdate {
    inserted,
    refreshed,
    moved,
    table_full,
    rejected,
    static_conflict,
};

class MacTableEntry {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    [[nodiscard]] constexpr routing::PortId port() const noexcept {
        return port_;
    }

    [[nodiscard]] constexpr MacEntryType type() const noexcept {
        return type_;
    }

    [[nodiscard]] constexpr TimePoint learned_at() const noexcept {
        return learned_at_;
    }

    [[nodiscard]] constexpr bool is_static() const noexcept {
        return type_ == MacEntryType::static_entry;
    }

    [[nodiscard]] constexpr bool operator==(
        const MacTableEntry& other) const noexcept {
        return port_ == other.port_ && type_ == other.type_ &&
               learned_at_ == other.learned_at_;
    }

private:
    friend class MacTable;

    constexpr MacTableEntry(
        const routing::PortId port,
        const MacEntryType type,
        const TimePoint learned_at) noexcept
        : port_{port}, type_{type}, learned_at_{learned_at} {}

    routing::PortId port_;
    MacEntryType type_;
    TimePoint learned_at_;
};

class MacTable {
public:
    using Clock = MacTableEntry::Clock;
    using TimePoint = MacTableEntry::TimePoint;

    explicit MacTable(std::size_t capacity);

    [[nodiscard]] MacTableUpdate learn(
        const network::VlanId& vlan,
        const network::MacAddress& mac_address,
        routing::PortId ingress_port);

    [[nodiscard]] MacTableUpdate learn(
        const network::VlanId& vlan,
        const network::MacAddress& mac_address,
        routing::PortId ingress_port,
        TimePoint learned_at);

    [[nodiscard]] MacTableUpdate add_static(
        const network::VlanId& vlan,
        const network::MacAddress& mac_address,
        routing::PortId port);

    [[nodiscard]] std::optional<MacTableEntry> lookup(
        const network::VlanId& vlan,
        const network::MacAddress& mac_address) const;

    [[nodiscard]] bool remove(
        const network::VlanId& vlan,
        const network::MacAddress& mac_address);

    [[nodiscard]] constexpr std::size_t capacity() const noexcept {
        return capacity_;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return entries_.size();
    }

    [[nodiscard]] bool empty() const noexcept {
        return entries_.empty();
    }

private:
    struct Key {
        network::VlanId vlan;
        network::MacAddress mac_address;
    };

    struct KeyLess {
        [[nodiscard]] bool operator()(const Key& left, const Key& right) const;
    };

    [[nodiscard]] MacTableUpdate update(
        const Key& key,
        routing::PortId port,
        MacEntryType type,
        TimePoint learned_at);

    std::size_t capacity_;
    std::map<Key, MacTableEntry, KeyLess> entries_;
};

}  // namespace silicon_switch::switching
