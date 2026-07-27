#pragma once

#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/mac_address.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace silicon_switch::network {

class EthernetFrame {
public:
    using Payload = std::vector<std::uint8_t>;

    static constexpr std::size_t header_size = 14U;
    static constexpr std::size_t maximum_payload_size = 1500U;

    [[nodiscard]] static std::optional<EthernetFrame> create(
        MacAddress destination,
        MacAddress source,
        EtherType ether_type,
        Payload payload);

    [[nodiscard]] static std::optional<EthernetFrame>
    parse(std::span<const std::uint8_t> bytes);

    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    [[nodiscard]] const MacAddress& destination() const noexcept {
        return destination_;
    }

    [[nodiscard]] const MacAddress& source() const noexcept {
        return source_;
    }

    [[nodiscard]] EtherType ether_type() const noexcept {
        return ether_type_;
    }

    [[nodiscard]] const Payload& payload() const noexcept {
        return payload_;
    }

    bool operator==(const EthernetFrame&) const = default;

private:
    struct ValidatedTag {};

    EthernetFrame(
        MacAddress destination,
        MacAddress source,
        EtherType ether_type,
        Payload payload,
        ValidatedTag);

    MacAddress destination_;
    MacAddress source_;
    EtherType ether_type_;
    Payload payload_;
};

}  // namespace silicon_switch::network
