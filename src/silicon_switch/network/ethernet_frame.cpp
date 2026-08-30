#include "silicon_switch/network/ethernet_frame.hpp"

#include "silicon_switch/network/byte_order.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include "silicon_switch/network/byte_span.hpp"
#include <utility>
#include <vector>

namespace silicon_switch::network {
namespace {

constexpr std::size_t destination_offset = 0U;
constexpr std::size_t source_offset =
    destination_offset + MacAddress::byte_count;
constexpr std::size_t ether_type_offset =
    source_offset + MacAddress::byte_count;
constexpr std::size_t vlan_control_information_offset =
    ether_type_offset + sizeof(std::uint16_t);
constexpr std::size_t inner_ether_type_offset =
    vlan_control_information_offset + sizeof(std::uint16_t);

[[nodiscard]] MacAddress read_mac_address(
    const ByteView bytes,
    const std::size_t offset) {
    MacAddress::Bytes address_bytes{};
    std::copy_n(
        bytes.begin() + static_cast<std::ptrdiff_t>(offset),
        MacAddress::byte_count,
        address_bytes.begin());
    return MacAddress{address_bytes};
}

void write_mac_address(
    const MacAddress& address,
    const MutableByteView bytes,
    const std::size_t offset) {
    std::copy(
        address.bytes().begin(),
        address.bytes().end(),
        bytes.begin() + static_cast<std::ptrdiff_t>(offset));
}

}  // namespace

std::optional<EthernetFrame> EthernetFrame::create(
    MacAddress destination,
    MacAddress source,
    const EtherType ether_type,
    Payload payload,
    std::optional<VlanTag> vlan_tag) {
    if (payload.size() > maximum_payload_size) {
        return std::nullopt;
    }

    return EthernetFrame{
        std::move(destination),
        std::move(source),
        ether_type,
        std::move(payload),
        std::move(vlan_tag),
        ValidatedTag{},
    };
}

std::optional<EthernetFrame> EthernetFrame::parse(
    const ByteView bytes) {
    if (bytes.size() < header_size) {
        return std::nullopt;
    }

    const auto outer_ether_type_value =
        wire::read_big_endian<std::uint16_t>(bytes, ether_type_offset);
    if (!outer_ether_type_value.has_value()) {
        return std::nullopt;
    }

    std::size_t payload_offset = header_size;
    std::uint16_t ether_type_value = *outer_ether_type_value;
    std::optional<VlanTag> vlan_tag;

    if (ether_type_value ==
        static_cast<std::uint16_t>(EtherType::vlan_tagged)) {
        if (bytes.size() < tagged_header_size) {
            return std::nullopt;
        }

        const auto tag_control_information =
            wire::read_big_endian<std::uint16_t>(
                bytes,
                vlan_control_information_offset);
        const auto inner_ether_type =
            wire::read_big_endian<std::uint16_t>(
                bytes,
                inner_ether_type_offset);
        if (!tag_control_information.has_value() ||
            !inner_ether_type.has_value()) {
            return std::nullopt;
        }

        vlan_tag =
            VlanTag::from_tag_control_information(*tag_control_information);
        if (!vlan_tag.has_value()) {
            return std::nullopt;
        }

        ether_type_value = *inner_ether_type;
        payload_offset = tagged_header_size;
    }

    if (bytes.size() - payload_offset > maximum_payload_size) {
        return std::nullopt;
    }

    const auto payload_begin =
        bytes.begin() + static_cast<std::ptrdiff_t>(payload_offset);
    Payload payload{payload_begin, bytes.end()};

    return create(
        read_mac_address(bytes, destination_offset),
        read_mac_address(bytes, source_offset),
        static_cast<EtherType>(ether_type_value),
        std::move(payload),
        std::move(vlan_tag));
}

std::vector<std::uint8_t> EthernetFrame::serialize() const {
    const std::size_t serialized_header_size =
        vlan_tag_.has_value() ? tagged_header_size : header_size;
    std::vector<std::uint8_t> bytes(
        serialized_header_size + payload_.size());
    const MutableByteView output{bytes};

    write_mac_address(destination_, output, destination_offset);
    write_mac_address(source_, output, source_offset);
    const EtherType outer_ether_type =
        vlan_tag_.has_value() ? EtherType::vlan_tagged : ether_type_;
    const bool wrote_outer_ether_type =
        wire::write_big_endian<std::uint16_t>(
            static_cast<std::uint16_t>(outer_ether_type),
            output,
            ether_type_offset);

    if (!wrote_outer_ether_type) {
        return {};
    }

    if (vlan_tag_.has_value()) {
        const bool wrote_vlan_tag = wire::write_big_endian<std::uint16_t>(
            vlan_tag_->tag_control_information(),
            output,
            vlan_control_information_offset);
        const bool wrote_inner_ether_type =
            wire::write_big_endian<std::uint16_t>(
                static_cast<std::uint16_t>(ether_type_),
                output,
                inner_ether_type_offset);
        if (!wrote_vlan_tag || !wrote_inner_ether_type) {
            return {};
        }
    }

    std::copy(
        payload_.begin(),
        payload_.end(),
        bytes.begin() +
            static_cast<std::ptrdiff_t>(serialized_header_size));
    return bytes;
}

EthernetFrame::EthernetFrame(
    MacAddress destination,
    MacAddress source,
    const EtherType ether_type,
    Payload payload,
    std::optional<VlanTag> vlan_tag,
    ValidatedTag)
    : destination_{std::move(destination)},
      source_{std::move(source)},
      ether_type_{ether_type},
      payload_{std::move(payload)},
      vlan_tag_{std::move(vlan_tag)} {}

}  // namespace silicon_switch::network
