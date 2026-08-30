#pragma once

#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/vlan_id.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"

#include <variant>

namespace silicon_switch::switching {

enum class VlanIngressDropReason {
    tagged_frame_on_access_port,
    untagged_frame_without_native_vlan,
    vlan_not_allowed,
};

class AcceptedVlanIngress {
public:
    explicit constexpr AcceptedVlanIngress(
        const network::VlanId vlan) noexcept
        : vlan_{vlan} {}

    [[nodiscard]] constexpr network::VlanId vlan() const noexcept {
        return vlan_;
    }

private:
    network::VlanId vlan_;
};

class DroppedVlanIngress {
public:
    explicit constexpr DroppedVlanIngress(
        const VlanIngressDropReason reason) noexcept
        : reason_{reason} {}

    [[nodiscard]] constexpr VlanIngressDropReason reason() const noexcept {
        return reason_;
    }

private:
    VlanIngressDropReason reason_;
};

using VlanIngressResult =
    std::variant<AcceptedVlanIngress, DroppedVlanIngress>;

[[nodiscard]] VlanIngressResult classify_vlan_ingress(
    const network::EthernetFrame& frame,
    const VlanPortConfig& port_config);

}  // namespace silicon_switch::switching
