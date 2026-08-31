#pragma once

#include "silicon_switch/network/byte_span.hpp"
#include "silicon_switch/transport/socket_handle.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace silicon_switch::transport {

enum class TcpErrorCode { system_error, disconnected, message_too_large };
struct TcpError { TcpErrorCode code; SocketError socket_error; };

class TcpConnection {
public:
    explicit TcpConnection(SocketHandle socket) noexcept : socket_{std::move(socket)} {}
    TcpConnection(TcpConnection&&) noexcept = default;
    TcpConnection& operator=(TcpConnection&&) noexcept = default;
    TcpConnection(const TcpConnection&) = delete;
    TcpConnection& operator=(const TcpConnection&) = delete;

    using ConnectResult=std::variant<TcpConnection,TcpError>;
    using ReceiveResult=std::variant<std::vector<std::uint8_t>,TcpError>;
    [[nodiscard]] static ConnectResult connect_ipv4(const std::string& address,std::uint16_t port);
    [[nodiscard]] bool valid() const noexcept{return socket_.valid();}
    [[nodiscard]] std::optional<TcpError> send_all(network::ByteView bytes);
    [[nodiscard]] ReceiveResult receive_exact(std::size_t size);
    [[nodiscard]] std::optional<TcpError> send_frame(network::ByteView payload);
    [[nodiscard]] ReceiveResult receive_frame(std::size_t maximum_size);
    [[nodiscard]] bool shutdown() noexcept{return socket_.shutdown_both();}

private:
    SocketHandle socket_;
};

class TcpServer {
public:
    explicit TcpServer(SocketHandle socket) noexcept : socket_{std::move(socket)} {}
    TcpServer(TcpServer&&) noexcept = default;
    TcpServer& operator=(TcpServer&&) noexcept = default;
    TcpServer(const TcpServer&) = delete;
    TcpServer& operator=(const TcpServer&) = delete;

    using ListenResult=std::variant<TcpServer,TcpError>;
    using AcceptResult=std::variant<TcpConnection,TcpError>;
    [[nodiscard]] static ListenResult listen_ipv4(
        const std::string& address,std::uint16_t port,int backlog=16);
    [[nodiscard]] AcceptResult accept();
    [[nodiscard]] std::uint16_t local_port() const;

private:
    SocketHandle socket_;
};

}  // namespace silicon_switch::transport
