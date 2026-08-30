#pragma once

#include <cstdint>
#include <optional>

namespace silicon_switch::routing {

class PortId {
public:
    using Value = std::uint16_t;

    [[nodiscard]] static constexpr std::optional<PortId>
    create(const Value value) noexcept {
        if (value == 0U) {
            return std::nullopt;
        }

        return PortId{value, ValidatedTag{}};
    }

    [[nodiscard]] constexpr Value value() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr bool operator==(const PortId& other) const noexcept {
        return value_ == other.value_;
    }
    [[nodiscard]] constexpr bool operator!=(const PortId& other) const noexcept {
        return !(*this == other);
    }
    [[nodiscard]] constexpr bool operator<(const PortId& other) const noexcept {
        return value_ < other.value_;
    }

private:
    struct ValidatedTag {};

    explicit constexpr PortId(const Value value, ValidatedTag) noexcept
        : value_{value} {}

    Value value_;
};

}  // namespace silicon_switch::routing
