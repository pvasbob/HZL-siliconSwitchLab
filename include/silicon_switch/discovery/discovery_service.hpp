#pragma once

#include "silicon_switch/discovery/discovery_protocol.hpp"
#include "silicon_switch/discovery/lab_configuration.hpp"
#include "silicon_switch/transport/udp.hpp"

#include <cstdint>
#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace silicon_switch::discovery {

enum class DiscoveryServiceError {
    network_error,
    malformed_message,
    unexpected_message,
    request_mismatch,
};

class DiscoveryResponder {
public:
    DiscoveryResponder(transport::UdpSocket socket, DiscoveryMessage advertisement)
        : socket_{std::move(socket)}, advertisement_{std::move(advertisement)} {}

    [[nodiscard]] std::variant<std::string, DiscoveryServiceError> respond_once();
    [[nodiscard]] std::uint16_t local_port() const { return socket_.local_port(); }

private:
    transport::UdpSocket socket_;
    DiscoveryMessage advertisement_;
};

class DiscoveryClient {
public:
    explicit DiscoveryClient(transport::UdpSocket socket) : socket_{std::move(socket)} {}

    [[nodiscard]] std::variant<DiscoveredMachine, DiscoveryServiceError> discover_one(
        const std::string& destination, std::uint16_t port, std::uint64_t request_id);
    [[nodiscard]] std::variant<std::vector<DiscoveredMachine>, DiscoveryServiceError>
    discover_all(const std::string& destination,
                 std::uint16_t port,
                 std::uint64_t request_id,
                 std::size_t maximum_results = 4U);
    [[nodiscard]] std::optional<transport::UdpError> enable_broadcast() {
        return socket_.enable_broadcast();
    }
    [[nodiscard]] std::optional<transport::UdpError> set_receive_timeout(
        std::chrono::milliseconds timeout) {
        return socket_.set_receive_timeout(timeout);
    }

private:
    transport::UdpSocket socket_;
};

}  // namespace silicon_switch::discovery
