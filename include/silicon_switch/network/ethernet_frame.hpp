#pragma once

#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_tag.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include "silicon_switch/network/byte_span.hpp"
#include <vector>

namespace silicon_switch::network {

class EthernetFrame {
public:
    using Payload = std::vector<std::uint8_t>;

    static constexpr std::size_t header_size = 14U;
    static constexpr std::size_t tagged_header_size = 18U;
    static constexpr std::size_t maximum_payload_size = 1500U;

    [[nodiscard]] static std::optional<EthernetFrame> create(
        MacAddress destination,
        MacAddress source,
        EtherType ether_type,
        Payload payload,
        std::optional<VlanTag> vlan_tag = std::nullopt);

    [[nodiscard]] static std::optional<EthernetFrame>
    parse(ByteView bytes);

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

    [[nodiscard]] const std::optional<VlanTag>& vlan_tag() const noexcept {
        return vlan_tag_;
    }

    [[nodiscard]] const Payload& payload() const noexcept {
        return payload_;
    }

    [[nodiscard]] bool operator==(const EthernetFrame& other) const noexcept {
        return destination_ == other.destination_ && source_ == other.source_ &&
               ether_type_ == other.ether_type_ && payload_ == other.payload_ &&
               vlan_tag_ == other.vlan_tag_;
    }

private:
    struct ValidatedTag {};

    EthernetFrame(
        MacAddress destination,
        MacAddress source,
        EtherType ether_type,
        Payload payload,
        std::optional<VlanTag> vlan_tag,
        ValidatedTag);

    MacAddress destination_;
    MacAddress source_;
    EtherType ether_type_;
    Payload payload_;
    std::optional<VlanTag> vlan_tag_;
};

}  // namespace silicon_switch::network
