#pragma once

#include "silicon_switch/network/mac_address.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace silicon_switch::switching {

enum class PortSpeed : std::uint32_t {
    mbps_100 = 100U,
    gbps_1 = 1'000U,
    gbps_10 = 10'000U,
    gbps_25 = 25'000U,
    gbps_40 = 40'000U,
    gbps_100 = 100'000U,
};

struct PortCounters {
    std::uint64_t received_packets{0U};
    std::uint64_t received_bytes{0U};
    std::uint64_t transmitted_packets{0U};
    std::uint64_t transmitted_bytes{0U};
    std::uint64_t receive_errors{0U};
    std::uint64_t transmit_errors{0U};
    std::uint64_t receive_drops{0U};
    std::uint64_t transmit_drops{0U};

    [[nodiscard]] bool operator==(const PortCounters& other) const noexcept {
        return received_packets == other.received_packets &&
               received_bytes == other.received_bytes &&
               transmitted_packets == other.transmitted_packets &&
               transmitted_bytes == other.transmitted_bytes &&
               receive_errors == other.receive_errors &&
               transmit_errors == other.transmit_errors &&
               receive_drops == other.receive_drops &&
               transmit_drops == other.transmit_drops;
    }
};

class VirtualPort {
public:
    static constexpr std::size_t minimum_mtu = 64U;
    static constexpr std::size_t maximum_mtu = 9'216U;

    [[nodiscard]] static std::optional<VirtualPort> create(
        routing::PortId id,
        network::MacAddress mac_address,
        PortSpeed speed,
        std::size_t mtu,
        VlanPortConfig vlan_config);

    [[nodiscard]] constexpr routing::PortId id() const noexcept { return id_; }
    [[nodiscard]] const network::MacAddress& mac_address() const noexcept {
        return mac_address_;
    }
    [[nodiscard]] constexpr PortSpeed speed() const noexcept { return speed_; }
    [[nodiscard]] constexpr std::size_t mtu() const noexcept { return mtu_; }
    [[nodiscard]] constexpr bool admin_enabled() const noexcept {
        return admin_enabled_;
    }
    [[nodiscard]] constexpr bool link_up() const noexcept { return link_up_; }
    [[nodiscard]] constexpr bool operational() const noexcept {
        return admin_enabled_ && link_up_;
    }
    [[nodiscard]] const VlanPortConfig& vlan_config() const noexcept {
        return vlan_config_;
    }
    [[nodiscard]] const PortCounters& counters() const noexcept {
        return counters_;
    }

    void set_admin_enabled(bool enabled) noexcept { admin_enabled_ = enabled; }
    void set_link_up(bool up) noexcept { link_up_ = up; }
    void set_speed(PortSpeed speed) noexcept { speed_ = speed; }
    [[nodiscard]] bool set_mtu(std::size_t mtu) noexcept;
    void set_vlan_config(VlanPortConfig config);

    [[nodiscard]] bool receive(std::size_t packet_bytes) noexcept;
    [[nodiscard]] bool transmit(std::size_t packet_bytes) noexcept;
    void record_receive_error() noexcept { ++counters_.receive_errors; }
    void record_transmit_error() noexcept { ++counters_.transmit_errors; }
    void reset_counters() noexcept { counters_ = {}; }

private:
    VirtualPort(
        routing::PortId id,
        network::MacAddress mac_address,
        PortSpeed speed,
        std::size_t mtu,
        VlanPortConfig vlan_config);

    routing::PortId id_;
    network::MacAddress mac_address_;
    PortSpeed speed_;
    std::size_t mtu_;
    VlanPortConfig vlan_config_;
    bool admin_enabled_{false};
    bool link_up_{false};
    PortCounters counters_;
};

}  // namespace silicon_switch::switching
