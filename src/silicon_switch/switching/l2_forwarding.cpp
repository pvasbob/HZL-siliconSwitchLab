#include "silicon_switch/switching/l2_forwarding.hpp"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace silicon_switch::switching {
namespace {

L2ForwardingDecision flood(
    const L2ForwardingAction action,
    const routing::PortId ingress_port,
    const std::vector<routing::PortId>& vlan_ports) {
    L2ForwardingDecision::OutputPorts outputs;
    for (const auto port : vlan_ports) {
        if (port != ingress_port &&
            std::find(outputs.begin(), outputs.end(), port) == outputs.end()) {
            outputs.push_back(port);
        }
    }
    if (outputs.empty()) {
        return L2ForwardingDecision::drop(
            L2ForwardingDropReason::no_eligible_egress_port);
    }
    return L2ForwardingDecision::forward(action, std::move(outputs));
}

}  // namespace

L2ForwardingDecision L2ForwardingDecision::forward(
    const L2ForwardingAction action,
    OutputPorts output_ports) {
    return L2ForwardingDecision{
        action, std::move(output_ports), std::nullopt};
}

L2ForwardingDecision L2ForwardingDecision::drop(
    const L2ForwardingDropReason reason) {
    return L2ForwardingDecision{
        L2ForwardingAction::drop, {}, reason};
}

L2ForwardingDecision::L2ForwardingDecision(
    const L2ForwardingAction action,
    OutputPorts output_ports,
    const std::optional<L2ForwardingDropReason> drop_reason)
    : action_{action},
      output_ports_{std::move(output_ports)},
      drop_reason_{drop_reason} {}

L2ForwardingDecision decide_l2_forwarding(
    const network::EthernetFrame& frame,
    const network::VlanId vlan,
    const routing::PortId ingress_port,
    const std::vector<routing::PortId>& vlan_ports,
    const MacTable& mac_table) {
    if (frame.destination().is_broadcast()) {
        return flood(
            L2ForwardingAction::broadcast_flood, ingress_port, vlan_ports);
    }
    if (frame.destination().is_multicast()) {
        return flood(
            L2ForwardingAction::multicast_flood, ingress_port, vlan_ports);
    }

    const auto destination = mac_table.lookup(vlan, frame.destination());
    if (!destination.has_value()) {
        return flood(
            L2ForwardingAction::unknown_unicast_flood,
            ingress_port,
            vlan_ports);
    }
    if (destination->port() == ingress_port) {
        return L2ForwardingDecision::drop(
            L2ForwardingDropReason::same_ingress_port);
    }
    if (std::find(vlan_ports.begin(), vlan_ports.end(), destination->port()) ==
        vlan_ports.end()) {
        return L2ForwardingDecision::drop(
            L2ForwardingDropReason::destination_not_in_vlan);
    }
    return L2ForwardingDecision::forward(
        L2ForwardingAction::known_unicast, {destination->port()});
}

}  // namespace silicon_switch::switching
