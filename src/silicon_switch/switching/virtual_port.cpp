#include "silicon_switch/switching/virtual_port.hpp"

#include <utility>

namespace silicon_switch::switching {
namespace {

[[nodiscard]] bool is_valid_port_mac(
    const network::MacAddress& address) noexcept {
    constexpr network::MacAddress::Bytes unspecified_bytes{};
    return address != network::MacAddress{unspecified_bytes} &&
           address.is_unicast();
}

}  // namespace

std::optional<VirtualPort> VirtualPort::create(
    const routing::PortId id,
    network::MacAddress mac_address,
    const PortSpeed speed,
    const std::size_t mtu,
    VlanPortConfig vlan_config) {
    if (!is_valid_port_mac(mac_address) || mtu < minimum_mtu ||
        mtu > maximum_mtu) {
        return std::nullopt;
    }
    return VirtualPort{
        id,
        std::move(mac_address),
        speed,
        mtu,
        std::move(vlan_config)};
}

bool VirtualPort::set_mtu(const std::size_t mtu) noexcept {
    if (mtu < minimum_mtu || mtu > maximum_mtu) {
        return false;
    }
    mtu_ = mtu;
    return true;
}

void VirtualPort::set_vlan_config(VlanPortConfig config) {
    vlan_config_ = std::move(config);
}

bool VirtualPort::receive(const std::size_t packet_bytes) noexcept {
    if (!operational()) {
        ++counters_.receive_drops;
        return false;
    }
    if (packet_bytes > mtu_) {
        ++counters_.receive_errors;
        ++counters_.receive_drops;
        return false;
    }
    ++counters_.received_packets;
    counters_.received_bytes += packet_bytes;
    return true;
}

bool VirtualPort::transmit(const std::size_t packet_bytes) noexcept {
    if (!operational()) {
        ++counters_.transmit_drops;
        return false;
    }
    if (packet_bytes > mtu_) {
        ++counters_.transmit_errors;
        ++counters_.transmit_drops;
        return false;
    }
    ++counters_.transmitted_packets;
    counters_.transmitted_bytes += packet_bytes;
    return true;
}

VirtualPort::VirtualPort(
    const routing::PortId id,
    network::MacAddress mac_address,
    const PortSpeed speed,
    const std::size_t mtu,
    VlanPortConfig vlan_config)
    : id_{id},
      mac_address_{std::move(mac_address)},
      speed_{speed},
      mtu_{mtu},
      vlan_config_{std::move(vlan_config)} {}

}  // namespace silicon_switch::switching
