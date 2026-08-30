#pragma once

#include "silicon_switch/asic/switch_asic.hpp"

#include <cstddef>
#include <utility>

namespace silicon_switch::asic {

class MockSwitchAsic final : public SwitchAsic {
public:
    void set_status(AsicStatus status) noexcept { status_ = status; }
    void set_packet_result(PacketProcessingResult result) {
        packet_result_ = std::move(result);
    }
    [[nodiscard]] constexpr std::size_t operation_count() const noexcept {
        return operation_count_;
    }

    AsicStatus create_port(switching::VirtualPort, std::size_t) override;
    AsicStatus remove_port(routing::PortId) override;
    AsicStatus set_port_state(routing::PortId, bool, bool) override;
    AsicStatus create_vlan(network::VlanId) override;
    AsicStatus remove_vlan(network::VlanId) override;
    AsicStatus add_vlan_member(network::VlanId, routing::PortId) override;
    AsicStatus set_router_interface(network::VlanId, network::MacAddress) override;
    AsicStatus add_or_replace_route(routing::RouteEntry) override;
    AsicStatus remove_route(network::Ipv4Prefix) override;
    AsicStatus add_or_replace_neighbor(network::Ipv4Address, network::MacAddress,
                                       switching::MacTable::TimePoint) override;
    AsicStatus remove_neighbor(network::Ipv4Address) override;
    AsicStatus configure_faults(FaultInjectionConfig) override;
    bool has_port(routing::PortId) const override { return false; }
    bool has_vlan(network::VlanId) const override { return false; }
    std::optional<routing::RouteEntry> find_route(network::Ipv4Prefix) const override {
        return std::nullopt;
    }
    std::optional<network::MacAddress> find_neighbor(
        network::Ipv4Address, switching::MacTable::TimePoint) const override {
        return std::nullopt;
    }
    PacketProcessingResult process_packet(routing::PortId,std::vector<std::uint8_t>,
                                           switching::MacTable::TimePoint) override;
    std::optional<std::vector<std::uint8_t>> dequeue_packet(routing::PortId) override;
    TrafficStatisticsSnapshot counters() const noexcept override;

private:
    AsicStatus record() noexcept { ++operation_count_; return status_; }
    AsicStatus status_{AsicStatus::success};
    PacketProcessingResult packet_result_{};
    std::size_t operation_count_{0U};
    TrafficStatistics statistics_;
};

}  // namespace silicon_switch::asic
