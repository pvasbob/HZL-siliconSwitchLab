#include "silicon_switch/transport/udp.hpp"

#include <array>
#include <chrono>
#include <limits>
#include <utility>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

namespace silicon_switch::transport {
namespace {
std::variant<sockaddr_in, UdpError> endpoint(const std::string& address,
                                             const std::uint16_t port) {
    sockaddr_in value{};
    value.sin_family = AF_INET;
    value.sin_port = htons(port);
    if (inet_pton(AF_INET, address.c_str(), &value.sin_addr) != 1) {
        return UdpError{{0, "invalid IPv4 address"}};
    }
    return value;
}
}  // namespace

UdpSocket::BindResult UdpSocket::bind_ipv4(const std::string& address,
                                           const std::uint16_t port) {
    const auto target = endpoint(address, port);
    if (std::holds_alternative<UdpError>(target)) {
        return std::get<UdpError>(target);
    }
    SocketHandle socket{::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)};
    if (!socket.valid()) {
        return UdpError{last_socket_error()};
    }
    const auto& value = std::get<sockaddr_in>(target);
    if (::bind(socket.native_handle(), reinterpret_cast<const sockaddr*>(&value),
               static_cast<int>(sizeof(value))) != 0) {
        return UdpError{last_socket_error()};
    }
    return UdpSocket{std::move(socket)};
}

std::variant<std::size_t, UdpError> UdpSocket::send_to_ipv4(
    const std::string& address,
    const std::uint16_t port,
    const network::ByteView payload) {
    const auto target = endpoint(address, port);
    if (std::holds_alternative<UdpError>(target)) {
        return std::get<UdpError>(target);
    }
    if (payload.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        return UdpError{{0, "datagram is too large"}};
    }
    const auto& value = std::get<sockaddr_in>(target);
#ifdef _WIN32
    const int sent = ::sendto(socket_.native_handle(),
                              reinterpret_cast<const char*>(payload.begin()),
                              static_cast<int>(payload.size()), 0,
                              reinterpret_cast<const sockaddr*>(&value),
                              static_cast<int>(sizeof(value)));
#else
    const auto sent = ::sendto(socket_.native_handle(), payload.begin(), payload.size(), 0,
                               reinterpret_cast<const sockaddr*>(&value), sizeof(value));
#endif
    if (sent < 0) {
        return UdpError{last_socket_error()};
    }
    return static_cast<std::size_t>(sent);
}

UdpSocket::ReceiveResult UdpSocket::receive(const std::size_t maximum_size) {
    std::vector<std::uint8_t> payload(maximum_size);
    sockaddr_in source{};
#ifdef _WIN32
    int source_size = static_cast<int>(sizeof(source));
    const int received = ::recvfrom(socket_.native_handle(),
                                    reinterpret_cast<char*>(payload.data()),
                                    static_cast<int>(payload.size()), 0,
                                    reinterpret_cast<sockaddr*>(&source), &source_size);
#else
    socklen_t source_size = sizeof(source);
    const auto received = ::recvfrom(socket_.native_handle(), payload.data(), payload.size(), 0,
                                     reinterpret_cast<sockaddr*>(&source), &source_size);
#endif
    if (received < 0) {
        return UdpError{last_socket_error()};
    }
    payload.resize(static_cast<std::size_t>(received));
    std::array<char, INET_ADDRSTRLEN> address{};
    if (inet_ntop(AF_INET, &source.sin_addr, address.data(),
                  static_cast<socklen_t>(address.size())) == nullptr) {
        return UdpError{last_socket_error()};
    }
    return UdpDatagram{address.data(), ntohs(source.sin_port), std::move(payload)};
}

std::uint16_t UdpSocket::local_port() const {
    sockaddr_in address{};
#ifdef _WIN32
    int size = static_cast<int>(sizeof(address));
#else
    socklen_t size = sizeof(address);
#endif
    if (getsockname(socket_.native_handle(), reinterpret_cast<sockaddr*>(&address), &size) != 0) {
        return 0U;
    }
    return ntohs(address.sin_port);
}

std::optional<UdpError> UdpSocket::enable_broadcast() {
    const int enabled = 1;
    if (setsockopt(socket_.native_handle(), SOL_SOCKET, SO_BROADCAST,
                   reinterpret_cast<const char*>(&enabled),
                   static_cast<int>(sizeof(enabled))) != 0) {
        return UdpError{last_socket_error()};
    }
    return std::nullopt;
}

std::optional<UdpError> UdpSocket::set_receive_timeout(
    const std::chrono::milliseconds timeout) {
#ifdef _WIN32
    const auto value = static_cast<DWORD>(timeout.count());
#else
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(timeout);
    const auto remainder = timeout - seconds;
    const timeval value{static_cast<time_t>(seconds.count()),
                        static_cast<suseconds_t>(remainder.count() * 1000)};
#endif
    if (setsockopt(socket_.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&value),
                   static_cast<int>(sizeof(value))) != 0) {
        return UdpError{last_socket_error()};
    }
    return std::nullopt;
}

}  // namespace silicon_switch::transport
