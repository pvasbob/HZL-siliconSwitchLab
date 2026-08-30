#include "silicon_switch/switching/vlan_port_config.hpp"

#include "silicon_switch/network/vlan_id.hpp"

#include <optional>
#include <set>
#include <utility>

namespace silicon_switch::switching {

VlanPortConfig VlanPortConfig::access(const network::VlanId vlan) {
    return VlanPortConfig{
        VlanPortMode::access,
        vlan,
        AllowedVlans{vlan},
        std::nullopt};
}

std::optional<VlanPortConfig> VlanPortConfig::trunk(
    AllowedVlans allowed_vlans,
    const std::optional<network::VlanId> native_vlan) {
    if (allowed_vlans.empty() ||
        (native_vlan.has_value() &&
         allowed_vlans.find(*native_vlan) == allowed_vlans.end())) {
        return std::nullopt;
    }

    return VlanPortConfig{
        VlanPortMode::trunk,
        std::nullopt,
        std::move(allowed_vlans),
        native_vlan};
}

VlanPortConfig::VlanPortConfig(
    const VlanPortMode mode,
    const std::optional<network::VlanId> access_vlan,
    AllowedVlans allowed_vlans,
    const std::optional<network::VlanId> native_vlan)
    : mode_{mode},
      access_vlan_{access_vlan},
      allowed_vlans_{std::move(allowed_vlans)},
      native_vlan_{native_vlan} {}

}  // namespace silicon_switch::switching
