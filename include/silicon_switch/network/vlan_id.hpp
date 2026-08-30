#pragma once

#include <cstdint>
#include <optional>

namespace silicon_switch::network {

class VlanId {
public:
    using Value = std::uint16_t;

    static constexpr Value minimum_value = 1U;
    static constexpr Value maximum_value = 4094U;

    [[nodiscard]] static constexpr std::optional<VlanId> create(
        const Value value) noexcept {
        if (value < minimum_value || value > maximum_value) {
            return std::nullopt;
        }

        return VlanId{value, ValidatedTag{}};
    }

    [[nodiscard]] constexpr Value value() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr bool operator==(const VlanId& other) const noexcept {
        return value_ == other.value_;
    }
    [[nodiscard]] constexpr bool operator!=(const VlanId& other) const noexcept {
        return !(*this == other);
    }
    [[nodiscard]] constexpr bool operator<(const VlanId& other) const noexcept {
        return value_ < other.value_;
    }

private:
    struct ValidatedTag {};

    explicit constexpr VlanId(
        const Value value,
        ValidatedTag) noexcept
        : value_{value} {}

    Value value_;
};

}  // namespace silicon_switch::network
