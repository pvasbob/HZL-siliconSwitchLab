#pragma once

#include "silicon_switch/network/byte_span.hpp"
#include "silicon_switch/transport/socket_handle.hpp"

#include <cstddef>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace silicon_switch::transport {

struct UdpError {
    SocketError socket_error;
};

struct UdpDatagram {
    std::string source_address;
    std::uint16_t source_port;
    std::vector<std::uint8_t> payload;
};

class UdpSocket {
public:
    using BindResult = std::variant<UdpSocket, UdpError>;
    using ReceiveResult = std::variant<UdpDatagram, UdpError>;

    explicit UdpSocket(SocketHandle socket) noexcept : socket_{std::move(socket)} {}
    UdpSocket(UdpSocket&&) noexcept = default;
    UdpSocket& operator=(UdpSocket&&) noexcept = default;
    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;

    [[nodiscard]] static BindResult bind_ipv4(const std::string& address,
                                              std::uint16_t port);
    [[nodiscard]] std::variant<std::size_t, UdpError> send_to_ipv4(
        const std::string& address, std::uint16_t port, network::ByteView payload);
    [[nodiscard]] ReceiveResult receive(std::size_t maximum_size);
    [[nodiscard]] std::optional<UdpError> enable_broadcast();
    [[nodiscard]] std::optional<UdpError> set_receive_timeout(
        std::chrono::milliseconds timeout);
    [[nodiscard]] std::uint16_t local_port() const;

private:
    SocketHandle socket_;
};

}  // namespace silicon_switch::transport
