#pragma once

#include "silicon_switch/network/byte_span.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace silicon_switch::transport {

enum class ControlMessageType : std::uint16_t {
    create_port=1U, remove_port=2U, set_port_state=3U,
    create_vlan=10U, remove_vlan=11U, add_vlan_member=12U,
    set_router_interface=20U, add_or_replace_route=21U, remove_route=22U,
    add_or_replace_neighbor=23U, remove_neighbor=24U,
    configure_faults=30U, query_counters=40U, response=100U,
};

enum class ControlParseError {
    truncated, invalid_magic, unsupported_version, unknown_message_type,
    payload_too_large, length_mismatch,
};

class ControlMessage {
public:
    static constexpr std::uint16_t current_version=1U;
    static constexpr std::size_t header_size=16U;
    static constexpr std::size_t maximum_payload_size=1'048'576U;

    [[nodiscard]] static std::optional<ControlMessage> create(
        ControlMessageType type,std::uint32_t request_id,
        std::vector<std::uint8_t> payload={});
    [[nodiscard]] static std::variant<ControlMessage,ControlParseError> parse(
        network::ByteView bytes);
    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    [[nodiscard]] constexpr std::uint16_t version() const noexcept{return version_;}
    [[nodiscard]] constexpr ControlMessageType type() const noexcept{return type_;}
    [[nodiscard]] constexpr std::uint32_t request_id() const noexcept{return request_id_;}
    [[nodiscard]] const std::vector<std::uint8_t>& payload() const noexcept{return payload_;}

private:
    ControlMessage(ControlMessageType type,std::uint32_t request_id,
                   std::vector<std::uint8_t> payload);
    std::uint16_t version_{current_version};
    ControlMessageType type_;
    std::uint32_t request_id_;
    std::vector<std::uint8_t> payload_;
};

}  // namespace silicon_switch::transport
