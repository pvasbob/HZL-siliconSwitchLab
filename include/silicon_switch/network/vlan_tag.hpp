#pragma once

#include "silicon_switch/network/vlan_id.hpp"

#include <compare>
#include <cstdint>
#include <optional>

namespace silicon_switch::network {

class VlanTag {
public:
    using Priority = std::uint8_t;

    static constexpr Priority maximum_priority = 7U;

    [[nodiscard]] static constexpr std::optional<VlanTag> create(
        const VlanId vlan_id,
        const Priority priority = 0U,
        const bool drop_eligible = false) noexcept {
        if (priority > maximum_priority) {
            return std::nullopt;
        }

        return VlanTag{
            vlan_id,
            priority,
            drop_eligible,
            ValidatedTag{},
        };
    }

    [[nodiscard]] static constexpr std::optional<VlanTag>
    from_tag_control_information(const std::uint16_t value) noexcept {
        constexpr std::uint16_t vlan_id_mask = 0x0FFFU;
        constexpr std::uint16_t drop_eligible_mask = 0x1000U;
        constexpr unsigned int priority_shift = 13U;

        const auto vlan_id =
            VlanId::create(static_cast<VlanId::Value>(value & vlan_id_mask));
        if (!vlan_id.has_value()) {
            return std::nullopt;
        }

        const auto priority =
            static_cast<Priority>(value >> priority_shift);
        const bool drop_eligible = (value & drop_eligible_mask) != 0U;
        return create(*vlan_id, priority, drop_eligible);
    }

    [[nodiscard]] constexpr std::uint16_t
    tag_control_information() const noexcept {
        constexpr unsigned int priority_shift = 13U;
        constexpr unsigned int drop_eligible_shift = 12U;

        return static_cast<std::uint16_t>(
            (static_cast<std::uint16_t>(priority_) << priority_shift) |
            (static_cast<std::uint16_t>(drop_eligible_) <<
             drop_eligible_shift) |
            vlan_id_.value());
    }

    [[nodiscard]] constexpr VlanId vlan_id() const noexcept {
        return vlan_id_;
    }

    [[nodiscard]] constexpr Priority priority() const noexcept {
        return priority_;
    }

    [[nodiscard]] constexpr bool drop_eligible() const noexcept {
        return drop_eligible_;
    }

    auto operator<=>(const VlanTag&) const noexcept = default;

private:
    struct ValidatedTag {};

    explicit constexpr VlanTag(
        const VlanId vlan_id,
        const Priority priority,
        const bool drop_eligible,
        ValidatedTag) noexcept
        : vlan_id_{vlan_id},
          priority_{priority},
          drop_eligible_{drop_eligible} {}

    VlanId vlan_id_;
    Priority priority_;
    bool drop_eligible_;
};

}  // namespace silicon_switch::network
