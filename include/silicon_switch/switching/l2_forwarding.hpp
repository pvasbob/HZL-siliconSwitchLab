#pragma once

#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/mac_table.hpp"

#include <optional>
#include <vector>

namespace silicon_switch::switching {

enum class L2ForwardingAction {
    known_unicast,
    unknown_unicast_flood,
    broadcast_flood,
    multicast_flood,
    drop,
};

enum class L2ForwardingDropReason {
    same_ingress_port,
    destination_not_in_vlan,
    no_eligible_egress_port,
};

class L2ForwardingDecision {
public:
    using OutputPorts = std::vector<routing::PortId>;

    [[nodiscard]] static L2ForwardingDecision forward(
        L2ForwardingAction action,
        OutputPorts output_ports);

    [[nodiscard]] static L2ForwardingDecision drop(
        L2ForwardingDropReason reason);

    [[nodiscard]] constexpr L2ForwardingAction action() const noexcept {
        return action_;
    }

    [[nodiscard]] const OutputPorts& output_ports() const noexcept {
        return output_ports_;
    }

    [[nodiscard]] const std::optional<L2ForwardingDropReason>&
    drop_reason() const noexcept {
        return drop_reason_;
    }

private:
    L2ForwardingDecision(
        L2ForwardingAction action,
        OutputPorts output_ports,
        std::optional<L2ForwardingDropReason> drop_reason);

    L2ForwardingAction action_;
    OutputPorts output_ports_;
    std::optional<L2ForwardingDropReason> drop_reason_;
};

[[nodiscard]] L2ForwardingDecision decide_l2_forwarding(
    const network::EthernetFrame& frame,
    network::VlanId vlan,
    routing::PortId ingress_port,
    const std::vector<routing::PortId>& vlan_ports,
    const MacTable& mac_table);

}  // namespace silicon_switch::switching
