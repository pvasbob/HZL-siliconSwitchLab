#pragma once

#include "silicon_switch/network/byte_span.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/mac_address.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace silicon_switch::network {

enum class ArpOperation : std::uint16_t {
    request = 1U,
    reply = 2U,
};

class ArpPacket {
public:
    static constexpr std::size_t serialized_size = 28U;

    [[nodiscard]] static std::optional<ArpPacket> create_request(
        MacAddress sender_mac,
        Ipv4Address sender_ip,
        Ipv4Address target_ip);

    [[nodiscard]] static std::optional<ArpPacket> create_reply(
        MacAddress sender_mac,
        Ipv4Address sender_ip,
        MacAddress target_mac,
        Ipv4Address target_ip);

    [[nodiscard]] static std::optional<ArpPacket> parse(ByteView bytes);

    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    [[nodiscard]] constexpr ArpOperation operation() const noexcept {
        return operation_;
    }

    [[nodiscard]] constexpr const MacAddress& sender_mac() const noexcept {
        return sender_mac_;
    }

    [[nodiscard]] constexpr Ipv4Address sender_ip() const noexcept {
        return sender_ip_;
    }

    [[nodiscard]] constexpr const MacAddress& target_mac() const noexcept {
        return target_mac_;
    }

    [[nodiscard]] constexpr Ipv4Address target_ip() const noexcept {
        return target_ip_;
    }

    [[nodiscard]] bool is_request() const noexcept {
        return operation_ == ArpOperation::request;
    }

    [[nodiscard]] bool is_reply() const noexcept {
        return operation_ == ArpOperation::reply;
    }

    [[nodiscard]] bool operator==(const ArpPacket& other) const noexcept {
        return operation_ == other.operation_ &&
               sender_mac_ == other.sender_mac_ &&
               sender_ip_ == other.sender_ip_ &&
               target_mac_ == other.target_mac_ &&
               target_ip_ == other.target_ip_;
    }

private:
    struct ValidatedTag {};

    ArpPacket(
        ArpOperation operation,
        MacAddress sender_mac,
        Ipv4Address sender_ip,
        MacAddress target_mac,
        Ipv4Address target_ip,
        ValidatedTag) noexcept;

    ArpOperation operation_;
    MacAddress sender_mac_;
    Ipv4Address sender_ip_;
    MacAddress target_mac_;
    Ipv4Address target_ip_;
};

}  // namespace silicon_switch::network
