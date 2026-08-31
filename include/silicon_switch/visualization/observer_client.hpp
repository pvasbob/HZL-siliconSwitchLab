#pragma once

#include "silicon_switch/transport/client_synchronizer.hpp"
#include "silicon_switch/transport/udp.hpp"
#include "silicon_switch/visualization/topology.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <variant>

namespace silicon_switch::visualization {

enum class ObserverResult {
    applied_snapshot,
    applied_delta,
    ignored,
    resync_required,
    malformed_update,
    malformed_scene,
    network_error,
};

class ObserverClient {
public:
    using BindResult = std::variant<ObserverClient, transport::UdpError>;

    explicit ObserverClient(transport::UdpSocket socket) : socket_{std::move(socket)} {}
    ObserverClient(ObserverClient&&) noexcept = default;
    ObserverClient& operator=(ObserverClient&&) noexcept = default;
    ObserverClient(const ObserverClient&) = delete;
    ObserverClient& operator=(const ObserverClient&) = delete;

    [[nodiscard]] static BindResult bind(const std::string& address, std::uint16_t port);
    [[nodiscard]] ObserverResult receive_next();
    [[nodiscard]] ObserverResult consume_datagram(network::ByteView bytes);
    [[nodiscard]] std::uint16_t local_port() const { return socket_.local_port(); }
    [[nodiscard]] const TopologyScene& scene() const noexcept { return scene_; }
    [[nodiscard]] const transport::ClientSynchronizer& synchronizer() const noexcept {
        return synchronizer_;
    }

private:
    transport::UdpSocket socket_;
    transport::ClientSynchronizer synchronizer_;
    TopologyScene scene_;
};

}  // namespace silicon_switch::visualization
