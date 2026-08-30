#include "silicon_switch/switching/vlan_ingress.hpp"

#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"

namespace silicon_switch::switching {

VlanIngressResult classify_vlan_ingress(
    const network::EthernetFrame& frame,
    const VlanPortConfig& port_config) {
    if (port_config.is_access()) {
        if (frame.vlan_tag().has_value()) {
            return DroppedVlanIngress{
                VlanIngressDropReason::tagged_frame_on_access_port};
        }
        return AcceptedVlanIngress{*port_config.access_vlan()};
    }

    if (!frame.vlan_tag().has_value()) {
        if (!port_config.native_vlan().has_value()) {
            return DroppedVlanIngress{
                VlanIngressDropReason::untagged_frame_without_native_vlan};
        }
        return AcceptedVlanIngress{*port_config.native_vlan()};
    }

    const network::VlanId vlan = frame.vlan_tag()->vlan_id();
    if (!port_config.allows(vlan)) {
        return DroppedVlanIngress{VlanIngressDropReason::vlan_not_allowed};
    }
    return AcceptedVlanIngress{vlan};
}

}  // namespace silicon_switch::switching
