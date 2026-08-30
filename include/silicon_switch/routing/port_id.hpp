#pragma once

#include <compare>
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

    auto operator<=>(const PortId&) const noexcept = default;

private:
    struct ValidatedTag {};

    explicit constexpr PortId(const Value value, ValidatedTag) noexcept
        : value_{value} {}

    Value value_;
};

}  // namespace silicon_switch::routing
