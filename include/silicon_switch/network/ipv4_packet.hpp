#pragma once

#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_address.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include "silicon_switch/network/byte_span.hpp"
#include <vector>

namespace silicon_switch::network {

class Ipv4Packet {
public:
    using Payload = std::vector<std::uint8_t>;

    static constexpr std::size_t header_size = 20U;
    static constexpr std::size_t maximum_packet_size = 65'535U;
    static constexpr std::size_t maximum_payload_size =
        maximum_packet_size - header_size;
    static constexpr std::uint8_t default_time_to_live = 64U;

    [[nodiscard]] static std::optional<Ipv4Packet> create(
        Ipv4Address source,
        Ipv4Address destination,
        IpProtocol protocol,
        Payload payload,
        std::uint8_t time_to_live = default_time_to_live,
        std::uint16_t identification = 0U,
        bool dont_fragment = true);

    [[nodiscard]] static std::optional<Ipv4Packet>
    parse(ByteView bytes);

    [[nodiscard]] std::vector<std::uint8_t> serialize() const;

    [[nodiscard]] const Ipv4Address& source() const noexcept {
        return source_;
    }

    [[nodiscard]] const Ipv4Address& destination() const noexcept {
        return destination_;
    }

    [[nodiscard]] IpProtocol protocol() const noexcept {
        return protocol_;
    }

    [[nodiscard]] const Payload& payload() const noexcept {
        return payload_;
    }

    [[nodiscard]] std::uint8_t time_to_live() const noexcept {
        return time_to_live_;
    }

    [[nodiscard]] std::uint16_t identification() const noexcept {
        return identification_;
    }

    [[nodiscard]] bool dont_fragment() const noexcept {
        return dont_fragment_;
    }

    [[nodiscard]] bool operator==(const Ipv4Packet& other) const noexcept {
        return source_ == other.source_ && destination_ == other.destination_ &&
               protocol_ == other.protocol_ && payload_ == other.payload_ &&
               time_to_live_ == other.time_to_live_ &&
               identification_ == other.identification_ &&
               dont_fragment_ == other.dont_fragment_;
    }

private:
    struct ValidatedTag {};

    Ipv4Packet(
        Ipv4Address source,
        Ipv4Address destination,
        IpProtocol protocol,
        Payload payload,
        std::uint8_t time_to_live,
        std::uint16_t identification,
        bool dont_fragment,
        ValidatedTag);

    Ipv4Address source_;
    Ipv4Address destination_;
    IpProtocol protocol_;
    Payload payload_;
    std::uint8_t time_to_live_;
    std::uint16_t identification_;
    bool dont_fragment_;
};

}  // namespace silicon_switch::network
