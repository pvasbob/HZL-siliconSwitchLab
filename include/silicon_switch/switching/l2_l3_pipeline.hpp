#pragma once

#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/mac_table.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"

#include <map>
#include <optional>
#include <vector>

namespace silicon_switch::switching {

enum class PipelineAction {
    switch_known_unicast,
    flood,
    route_ipv4,
    deliver_to_control_plane,
    drop,
};

enum class PipelineDropReason {
    tagged_frame_on_access_port,
    untagged_frame_without_native_vlan,
    vlan_not_allowed,
    invalid_source_mac,
    static_mac_conflict,
    same_ingress_port,
    destination_not_in_vlan,
    no_eligible_egress_port,
    unsupported_router_protocol,
};

class PipelineDecision {
public:
    using OutputPorts = std::vector<routing::PortId>;

    [[nodiscard]] static PipelineDecision accept(
        PipelineAction action,
        network::VlanId vlan,
        OutputPorts output_ports = {});

    [[nodiscard]] static PipelineDecision drop(
        std::optional<network::VlanId> vlan,
        PipelineDropReason reason);

    [[nodiscard]] constexpr PipelineAction action() const noexcept {
        return action_;
    }

    [[nodiscard]] const std::optional<network::VlanId>& vlan() const noexcept {
        return vlan_;
    }

    [[nodiscard]] const OutputPorts& output_ports() const noexcept {
        return output_ports_;
    }

    [[nodiscard]] const std::optional<PipelineDropReason>&
    drop_reason() const noexcept {
        return drop_reason_;
    }

private:
    PipelineDecision(
        PipelineAction action,
        std::optional<network::VlanId> vlan,
        OutputPorts output_ports,
        std::optional<PipelineDropReason> drop_reason);

    PipelineAction action_;
    std::optional<network::VlanId> vlan_;
    OutputPorts output_ports_;
    std::optional<PipelineDropReason> drop_reason_;
};

using RouterInterfaces =
    std::map<network::VlanId, network::MacAddress>;

[[nodiscard]] PipelineDecision process_l2_l3_ingress(
    const network::EthernetFrame& frame,
    routing::PortId ingress_port,
    const VlanPortConfig& port_config,
    const std::vector<routing::PortId>& vlan_ports,
    MacTable& mac_table,
    const RouterInterfaces& router_interfaces,
    MacTable::TimePoint now);

}  // namespace silicon_switch::switching
