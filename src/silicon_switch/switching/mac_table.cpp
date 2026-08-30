#include "silicon_switch/switching/mac_table.hpp"

#include "silicon_switch/network/mac_address.hpp"

#include <chrono>
#include <stdexcept>

namespace silicon_switch::switching {
namespace {

[[nodiscard]] bool is_learnable(
    const network::MacAddress& address) noexcept {
    constexpr network::MacAddress::Bytes unspecified_bytes{};
    return address != network::MacAddress{unspecified_bytes} &&
           address.is_unicast();
}

}  // namespace

MacTable::MacTable(
    const std::size_t capacity,
    const Duration entry_lifetime)
    : capacity_{capacity}, entry_lifetime_{entry_lifetime} {
    if (capacity_ == 0U) {
        throw std::invalid_argument{"MAC table capacity must be positive"};
    }
    if (entry_lifetime_ <= Duration::zero()) {
        throw std::invalid_argument{"MAC entry lifetime must be positive"};
    }
}

MacTableUpdate MacTable::learn(
    const network::VlanId& vlan,
    const network::MacAddress& mac_address,
    const routing::PortId ingress_port) {
    return learn(vlan, mac_address, ingress_port, Clock::now());
}

MacTableUpdate MacTable::learn(
    const network::VlanId& vlan,
    const network::MacAddress& mac_address,
    const routing::PortId ingress_port,
    const TimePoint learned_at) {
    if (!is_learnable(mac_address)) {
        return MacTableUpdate::rejected;
    }
    return update(
        Key{vlan, mac_address},
        ingress_port,
        MacEntryType::dynamic,
        learned_at);
}

MacTableUpdate MacTable::add_static(
    const network::VlanId& vlan,
    const network::MacAddress& mac_address,
    const routing::PortId port) {
    if (!is_learnable(mac_address)) {
        return MacTableUpdate::rejected;
    }
    return update(
        Key{vlan, mac_address},
        port,
        MacEntryType::static_entry,
        TimePoint{});
}

std::optional<MacTableEntry> MacTable::lookup(
    const network::VlanId& vlan,
    const network::MacAddress& mac_address) const {
    const auto entry = entries_.find(Key{vlan, mac_address});
    if (entry == entries_.end()) {
        return std::nullopt;
    }
    return entry->second;
}

bool MacTable::remove(
    const network::VlanId& vlan,
    const network::MacAddress& mac_address) {
    return entries_.erase(Key{vlan, mac_address}) != 0U;
}

std::size_t MacTable::expire(const TimePoint now) {
    std::size_t removed = 0U;
    for (auto entry = entries_.begin(); entry != entries_.end();) {
        const bool expired =
            !entry->second.is_static() &&
            now >= entry->second.learned_at() &&
            now - entry->second.learned_at() >= entry_lifetime_;
        if (expired) {
            entry = entries_.erase(entry);
            ++removed;
        } else {
            ++entry;
        }
    }
    return removed;
}

bool MacTable::KeyLess::operator()(
    const Key& left,
    const Key& right) const {
    if (left.vlan != right.vlan) {
        return left.vlan < right.vlan;
    }
    return left.mac_address < right.mac_address;
}

MacTableUpdate MacTable::update(
    const Key& key,
    const routing::PortId port,
    const MacEntryType type,
    const TimePoint learned_at) {
    const auto existing = entries_.find(key);
    if (existing == entries_.end()) {
        if (entries_.size() >= capacity_) {
            return MacTableUpdate::table_full;
        }
        entries_.emplace(key, MacTableEntry{port, type, learned_at});
        return MacTableUpdate::inserted;
    }

    if (existing->second.is_static() && type == MacEntryType::dynamic) {
        if (existing->second.port() != port) {
            return MacTableUpdate::static_conflict;
        }
        return MacTableUpdate::refreshed;
    }

    const bool moved = existing->second.port() != port;
    existing->second = MacTableEntry{port, type, learned_at};
    return moved ? MacTableUpdate::moved : MacTableUpdate::refreshed;
}

}  // namespace silicon_switch::switching
