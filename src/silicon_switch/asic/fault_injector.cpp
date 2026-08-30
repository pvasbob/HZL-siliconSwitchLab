#include "silicon_switch/asic/fault_injector.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace silicon_switch::asic {
namespace {

void validate(const FaultInjectionConfig& config) {
    if (config.latency < std::chrono::nanoseconds::zero()) {
        throw std::invalid_argument{"fault latency cannot be negative"};
    }
}

}  // namespace

FaultInjectedPacket::FaultInjectedPacket(
    std::vector<std::uint8_t> bytes,
    const std::chrono::nanoseconds latency,
    const bool corrupted)
    : bytes_{std::move(bytes)}, latency_{latency}, corrupted_{corrupted} {}

FaultInjector::FaultInjector(FaultInjectionConfig config)
    : config_{std::move(config)} {
    validate(config_);
}

FaultInjectionResult FaultInjector::process(
    const routing::PortId port,
    std::vector<std::uint8_t> packet) {
    ++processed_packets_;
    if (config_.failed_ports.find(port) != config_.failed_ports.end()) {
        return FaultDropReason::failed_port;
    }
    if (config_.resource_exhausted) {
        return FaultDropReason::resource_exhaustion;
    }
    if (config_.drop_every_nth_packet != 0U &&
        processed_packets_ % config_.drop_every_nth_packet == 0U) {
        return FaultDropReason::deterministic_packet_loss;
    }

    bool corrupted = false;
    if (!packet.empty() && config_.corrupt_every_nth_packet != 0U &&
        processed_packets_ % config_.corrupt_every_nth_packet == 0U) {
        packet.front() ^= 0x01U;
        corrupted = true;
    }
    return FaultInjectedPacket{
        std::move(packet), config_.latency, corrupted};
}

void FaultInjector::set_config(FaultInjectionConfig config) {
    validate(config);
    config_ = std::move(config);
}

}  // namespace silicon_switch::asic
