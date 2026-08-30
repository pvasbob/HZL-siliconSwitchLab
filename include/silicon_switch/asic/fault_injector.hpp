#pragma once

#include "silicon_switch/routing/port_id.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <set>
#include <variant>
#include <vector>

namespace silicon_switch::asic {

struct FaultInjectionConfig {
    std::set<routing::PortId> failed_ports;
    std::size_t drop_every_nth_packet{0U};
    std::size_t corrupt_every_nth_packet{0U};
    std::chrono::nanoseconds latency{0};
    bool resource_exhausted{false};
};

enum class FaultDropReason {
    failed_port,
    deterministic_packet_loss,
    resource_exhaustion,
};

class FaultInjectedPacket {
public:
    FaultInjectedPacket(
        std::vector<std::uint8_t> bytes,
        std::chrono::nanoseconds latency,
        bool corrupted);

    [[nodiscard]] const std::vector<std::uint8_t>& bytes() const noexcept {
        return bytes_;
    }
    [[nodiscard]] constexpr std::chrono::nanoseconds latency() const noexcept {
        return latency_;
    }
    [[nodiscard]] constexpr bool corrupted() const noexcept {
        return corrupted_;
    }

private:
    std::vector<std::uint8_t> bytes_;
    std::chrono::nanoseconds latency_;
    bool corrupted_;
};

using FaultInjectionResult =
    std::variant<FaultInjectedPacket, FaultDropReason>;

class FaultInjector {
public:
    explicit FaultInjector(FaultInjectionConfig config = {});

    [[nodiscard]] FaultInjectionResult process(
        routing::PortId port,
        std::vector<std::uint8_t> packet);

    void set_config(FaultInjectionConfig config);
    [[nodiscard]] const FaultInjectionConfig& config() const noexcept {
        return config_;
    }
    [[nodiscard]] constexpr std::uint64_t processed_packets() const noexcept {
        return processed_packets_;
    }
    void reset_sequence() noexcept { processed_packets_ = 0U; }

private:
    FaultInjectionConfig config_;
    std::uint64_t processed_packets_{0U};
};

}  // namespace silicon_switch::asic
