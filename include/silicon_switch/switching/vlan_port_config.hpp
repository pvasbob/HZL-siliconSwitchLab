#pragma once

#include "silicon_switch/network/vlan_id.hpp"

#include <optional>
#include <set>

namespace silicon_switch::switching {

enum class VlanPortMode {
    access,
    trunk,
};

class VlanPortConfig {
public:
    using AllowedVlans = std::set<network::VlanId>;

    [[nodiscard]] static VlanPortConfig access(network::VlanId vlan);

    [[nodiscard]] static std::optional<VlanPortConfig> trunk(
        AllowedVlans allowed_vlans,
        std::optional<network::VlanId> native_vlan = std::nullopt);

    [[nodiscard]] constexpr VlanPortMode mode() const noexcept {
        return mode_;
    }

    [[nodiscard]] constexpr bool is_access() const noexcept {
        return mode_ == VlanPortMode::access;
    }

    [[nodiscard]] constexpr bool is_trunk() const noexcept {
        return mode_ == VlanPortMode::trunk;
    }

    [[nodiscard]] const std::optional<network::VlanId>&
    access_vlan() const noexcept {
        return access_vlan_;
    }

    [[nodiscard]] const std::optional<network::VlanId>&
    native_vlan() const noexcept {
        return native_vlan_;
    }

    [[nodiscard]] const AllowedVlans& allowed_vlans() const noexcept {
        return allowed_vlans_;
    }

    [[nodiscard]] bool allows(const network::VlanId vlan) const {
        return allowed_vlans_.find(vlan) != allowed_vlans_.end();
    }

private:
    VlanPortConfig(
        VlanPortMode mode,
        std::optional<network::VlanId> access_vlan,
        AllowedVlans allowed_vlans,
        std::optional<network::VlanId> native_vlan);

    VlanPortMode mode_;
    std::optional<network::VlanId> access_vlan_;
    AllowedVlans allowed_vlans_;
    std::optional<network::VlanId> native_vlan_;
};

}  // namespace silicon_switch::switching
