#include "silicon_switch/switching/l2_l3_pipeline.hpp"

#include "silicon_switch/network/ether_type.hpp"
#include "silicon_switch/switching/l2_forwarding.hpp"
#include "silicon_switch/switching/vlan_ingress.hpp"

#include <optional>
#include <utility>
#include <variant>

namespace silicon_switch::switching {
namespace {

PipelineDropReason map_vlan_drop(const VlanIngressDropReason reason) {
    switch (reason) {
        case VlanIngressDropReason::tagged_frame_on_access_port:
            return PipelineDropReason::tagged_frame_on_access_port;
        case VlanIngressDropReason::untagged_frame_without_native_vlan:
            return PipelineDropReason::untagged_frame_without_native_vlan;
        case VlanIngressDropReason::vlan_not_allowed:
            return PipelineDropReason::vlan_not_allowed;
    }
    return PipelineDropReason::vlan_not_allowed;
}

PipelineDropReason map_l2_drop(const L2ForwardingDropReason reason) {
    switch (reason) {
        case L2ForwardingDropReason::same_ingress_port:
            return PipelineDropReason::same_ingress_port;
        case L2ForwardingDropReason::destination_not_in_vlan:
            return PipelineDropReason::destination_not_in_vlan;
        case L2ForwardingDropReason::no_eligible_egress_port:
            return PipelineDropReason::no_eligible_egress_port;
    }
    return PipelineDropReason::no_eligible_egress_port;
}

}  // namespace

PipelineDecision PipelineDecision::accept(
    const PipelineAction action,
    const network::VlanId vlan,
    OutputPorts output_ports) {
    return PipelineDecision{
        action, vlan, std::move(output_ports), std::nullopt};
}

PipelineDecision PipelineDecision::drop(
    const std::optional<network::VlanId> vlan,
    const PipelineDropReason reason) {
    return PipelineDecision{
        PipelineAction::drop, vlan, {}, reason};
}

PipelineDecision::PipelineDecision(
    const PipelineAction action,
    const std::optional<network::VlanId> vlan,
    OutputPorts output_ports,
    const std::optional<PipelineDropReason> drop_reason)
    : action_{action},
      vlan_{vlan},
      output_ports_{std::move(output_ports)},
      drop_reason_{drop_reason} {}

PipelineDecision process_l2_l3_ingress(
    const network::EthernetFrame& frame,
    const routing::PortId ingress_port,
    const VlanPortConfig& port_config,
    const std::vector<routing::PortId>& vlan_ports,
    MacTable& mac_table,
    const RouterInterfaces& router_interfaces,
    const MacTable::TimePoint now) {
    const auto ingress = classify_vlan_ingress(frame, port_config);
    if (const auto* dropped = std::get_if<DroppedVlanIngress>(&ingress)) {
        return PipelineDecision::drop(
            std::nullopt, map_vlan_drop(dropped->reason()));
    }
    const auto vlan = std::get<AcceptedVlanIngress>(ingress).vlan();

    const auto learning =
        mac_table.learn(vlan, frame.source(), ingress_port, now);
    if (learning == MacTableUpdate::rejected) {
        return PipelineDecision::drop(
            vlan, PipelineDropReason::invalid_source_mac);
    }
    if (learning == MacTableUpdate::static_conflict) {
        return PipelineDecision::drop(
            vlan, PipelineDropReason::static_mac_conflict);
    }

    const auto router = router_interfaces.find(vlan);
    if (router != router_interfaces.end() &&
        frame.destination() == router->second) {
        if (frame.ether_type() == network::EtherType::ipv4) {
            return PipelineDecision::accept(PipelineAction::route_ipv4, vlan);
        }
        if (frame.ether_type() == network::EtherType::arp) {
            return PipelineDecision::accept(
                PipelineAction::deliver_to_control_plane, vlan);
        }
        return PipelineDecision::drop(
            vlan, PipelineDropReason::unsupported_router_protocol);
    }

    const auto l2 = decide_l2_forwarding(
        frame, vlan, ingress_port, vlan_ports, mac_table);
    if (l2.action() == L2ForwardingAction::drop) {
        return PipelineDecision::drop(
            vlan, map_l2_drop(*l2.drop_reason()));
    }
    if (l2.action() == L2ForwardingAction::known_unicast) {
        return PipelineDecision::accept(
            PipelineAction::switch_known_unicast,
            vlan,
            l2.output_ports());
    }
    return PipelineDecision::accept(
        PipelineAction::flood, vlan, l2.output_ports());
}

}  // namespace silicon_switch::switching
