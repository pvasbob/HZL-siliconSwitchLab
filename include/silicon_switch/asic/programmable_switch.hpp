#pragma once

#include "silicon_switch/asic/switch_asic.hpp"

#include <memory>

namespace silicon_switch::asic {

class ProgrammableSwitch {
public:
    explicit ProgrammableSwitch(std::unique_ptr<SwitchAsic> asic);

    [[nodiscard]] SwitchAsic& hardware() noexcept { return *asic_; }
    [[nodiscard]] const SwitchAsic& hardware() const noexcept { return *asic_; }

    AsicStatus create_port(switching::VirtualPort port, std::size_t queue_capacity);
    AsicStatus remove_port(routing::PortId port);
    AsicStatus set_port_state(routing::PortId port, bool admin_enabled, bool link_up);
    AsicStatus create_vlan(network::VlanId vlan);
    AsicStatus remove_vlan(network::VlanId vlan);
    AsicStatus add_vlan_member(network::VlanId vlan, routing::PortId port);
    AsicStatus set_router_interface(network::VlanId vlan, network::MacAddress mac);
    AsicStatus add_or_replace_route(routing::RouteEntry route);
    AsicStatus remove_route(network::Ipv4Prefix prefix);
    AsicStatus add_or_replace_neighbor(
        network::Ipv4Address address,
        network::MacAddress mac,
        switching::MacTable::TimePoint learned_at);
    AsicStatus remove_neighbor(network::Ipv4Address address);
    AsicStatus configure_faults(FaultInjectionConfig config);
    [[nodiscard]] bool has_port(routing::PortId port) const;
    [[nodiscard]] bool has_vlan(network::VlanId vlan) const;
    [[nodiscard]] std::optional<routing::RouteEntry> find_route(
        network::Ipv4Prefix prefix) const;
    [[nodiscard]] std::optional<network::MacAddress> find_neighbor(
        network::Ipv4Address address,
        switching::MacTable::TimePoint now) const;
    PacketProcessingResult process_packet(
        routing::PortId ingress_port,
        std::vector<std::uint8_t> frame,
        switching::MacTable::TimePoint now);
    std::optional<std::vector<std::uint8_t>> dequeue_packet(routing::PortId port);
    [[nodiscard]] TrafficStatisticsSnapshot counters() const noexcept;

private:
    std::unique_ptr<SwitchAsic> asic_;
};

}  // namespace silicon_switch::asic
