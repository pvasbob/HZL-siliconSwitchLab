#pragma once

#include "silicon_switch/asic/fault_injector.hpp"
#include "silicon_switch/asic/traffic_statistics.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/routing/route_entry.hpp"
#include "silicon_switch/switching/mac_table.hpp"
#include "silicon_switch/switching/virtual_port.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace silicon_switch::asic {

enum class AsicStatus {
    success,
    already_exists,
    not_found,
    invalid_argument,
    dependency_missing,
    resource_exhausted,
};

enum class PacketDisposition {
    switched,
    flooded,
    routed,
    delivered_to_control_plane,
    dropped,
};

enum class PacketDropReason {
    none,
    ingress_port_unavailable,
    malformed_frame,
    vlan_rejected,
    forwarding_filtered,
    malformed_ipv4,
    route_miss,
    ttl_expired,
    neighbor_miss,
    egress_port_unavailable,
    queue_congestion,
    injected_fault,
    resource_exhaustion,
};

struct PacketProcessingResult {
    PacketDisposition disposition{PacketDisposition::dropped};
    PacketDropReason drop_reason{PacketDropReason::none};
    std::vector<routing::PortId> output_ports;
    std::chrono::nanoseconds injected_latency{0};
};

class SwitchAsic {
public:
    virtual ~SwitchAsic() = default;

    virtual AsicStatus create_port(
        switching::VirtualPort port,
        std::size_t queue_capacity) = 0;
    virtual AsicStatus remove_port(routing::PortId port) = 0;
    virtual AsicStatus set_port_state(
        routing::PortId port,
        bool admin_enabled,
        bool link_up) = 0;
    virtual AsicStatus create_vlan(network::VlanId vlan) = 0;
    virtual AsicStatus remove_vlan(network::VlanId vlan) = 0;
    virtual AsicStatus add_vlan_member(
        network::VlanId vlan,
        routing::PortId port) = 0;
    virtual AsicStatus set_router_interface(
        network::VlanId vlan,
        network::MacAddress mac_address) = 0;
    virtual AsicStatus add_or_replace_route(routing::RouteEntry route) = 0;
    virtual AsicStatus remove_route(network::Ipv4Prefix prefix) = 0;
    virtual AsicStatus add_or_replace_neighbor(
        network::Ipv4Address address,
        network::MacAddress mac_address,
        switching::MacTable::TimePoint learned_at) = 0;
    virtual AsicStatus remove_neighbor(network::Ipv4Address address) = 0;
    virtual AsicStatus configure_faults(FaultInjectionConfig config) = 0;
    [[nodiscard]] virtual bool has_port(routing::PortId port) const = 0;
    [[nodiscard]] virtual bool has_vlan(network::VlanId vlan) const = 0;
    [[nodiscard]] virtual std::optional<routing::RouteEntry> find_route(
        network::Ipv4Prefix prefix) const = 0;
    [[nodiscard]] virtual std::optional<network::MacAddress> find_neighbor(
        network::Ipv4Address address,
        switching::MacTable::TimePoint now) const = 0;
    virtual PacketProcessingResult process_packet(
        routing::PortId ingress_port,
        std::vector<std::uint8_t> frame,
        switching::MacTable::TimePoint now) = 0;
    virtual std::optional<std::vector<std::uint8_t>> dequeue_packet(
        routing::PortId egress_port) = 0;
    virtual TrafficStatisticsSnapshot counters() const noexcept = 0;
};

}  // namespace silicon_switch::asic
