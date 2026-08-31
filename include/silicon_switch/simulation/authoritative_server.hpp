#pragma once

#include "silicon_switch/simulation/backend.hpp"
#include "silicon_switch/transport/state_update_protocol.hpp"
#include "silicon_switch/transport/udp.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace silicon_switch::simulation {

struct ObserverEndpoint {
    std::string address;
    std::uint16_t port;
};

enum class PublishError { serialization_failed, socket_error };

class AuthoritativeServer {
public:
    AuthoritativeServer(std::unique_ptr<SimulationBackend> backend,
                        transport::UdpSocket socket,
                        std::uint64_t session_id,
                        std::size_t snapshot_interval = 60U);

    void add_observer(ObserverEndpoint endpoint);
    [[nodiscard]] std::variant<transport::StateUpdate, PublishError> next_update(
        std::uint64_t timestamp_microseconds);
    [[nodiscard]] std::variant<std::size_t, PublishError> publish_next(
        std::uint64_t timestamp_microseconds);
    [[nodiscard]] std::uint64_t revision() const noexcept { return revision_; }
    [[nodiscard]] std::uint64_t sequence() const noexcept { return sequence_; }
    [[nodiscard]] std::string_view backend_name() const noexcept { return backend_->name(); }

private:
    std::unique_ptr<SimulationBackend> backend_;
    transport::UdpSocket socket_;
    std::vector<ObserverEndpoint> observers_;
    std::uint64_t session_id_;
    std::size_t snapshot_interval_;
    std::uint64_t sequence_{0U};
    std::uint64_t revision_{0U};
};

}  // namespace silicon_switch::simulation
