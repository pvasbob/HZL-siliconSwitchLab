#include "silicon_switch/asic/software_asic.hpp"
#include "silicon_switch/network/ethernet_frame.hpp"
#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/network/ipv4_prefix.hpp"
#include "silicon_switch/routing/route_entry.hpp"
#include "silicon_switch/switching/virtual_port.hpp"
#include "silicon_switch/switching/vlan_port_config.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace asic = silicon_switch::asic;
namespace network = silicon_switch::network;
namespace routing = silicon_switch::routing;
namespace switching = silicon_switch::switching;
namespace {

routing::PortId port(const std::uint16_t value) {
    return routing::PortId::create(value).value();
}

network::VlanId vlan(const std::uint16_t value) {
    return network::VlanId::create(value).value();
}

network::MacAddress mac(const std::uint8_t suffix) {
    return network::MacAddress{{0x02U, 0U, 0U, 0U, 0U, suffix}};
}

network::Ipv4Address ip(const std::uint32_t value) {
    return network::Ipv4Address{value};
}

switching::VirtualPort make_port(const std::uint16_t id,
                                 const std::uint16_t vlan_id) {
    return switching::VirtualPort::create(
               port(id), mac(0xFEU), switching::PortSpeed::gbps_10, 1500U,
               switching::VlanPortConfig::access(vlan(vlan_id)))
        .value();
}

network::EthernetFrame frame(network::MacAddress source,
                             network::MacAddress destination,
                             std::vector<std::uint8_t> payload = {1U}) {
    return network::EthernetFrame::create(destination, source,
                                          network::EtherType::ipv4,
                                          std::move(payload))
        .value();
}

void event(const std::string& phase, const bool passed,
           const std::string& evidence) {
    std::cout << "{\"phase\":\"" << phase << "\",\"passed\":"
              << (passed ? "true" : "false") << ",\"evidence\":\""
              << evidence << "\"}\n";
}

bool configure(asic::SoftwareAsic& device) {
    return device.create_vlan(vlan(10U)) == asic::AsicStatus::success &&
           device.create_vlan(vlan(20U)) == asic::AsicStatus::success &&
           device.create_port(make_port(1U, 10U), 4U) == asic::AsicStatus::success &&
           device.create_port(make_port(2U, 10U), 1U) == asic::AsicStatus::success &&
           device.create_port(make_port(3U, 20U), 4U) == asic::AsicStatus::success &&
           device.set_port_state(port(1U), true, true) == asic::AsicStatus::success &&
           device.set_port_state(port(2U), true, true) == asic::AsicStatus::success &&
           device.set_port_state(port(3U), true, true) == asic::AsicStatus::success &&
           device.add_vlan_member(vlan(10U), port(1U)) == asic::AsicStatus::success &&
           device.add_vlan_member(vlan(10U), port(2U)) == asic::AsicStatus::success &&
           device.add_vlan_member(vlan(20U), port(3U)) == asic::AsicStatus::success &&
           device.set_router_interface(vlan(10U), mac(0xFEU)) ==
               asic::AsicStatus::success;
}

}  // namespace

int main() {
    asic::SoftwareAsic device{32U};
    if (!configure(device)) {
        std::cerr << "demo configuration failed\n";
        return EXIT_FAILURE;
    }
    const auto now = switching::MacTable::TimePoint{};
    const auto broadcast = network::MacAddress{
        {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU}};

    const auto isolated = device.process_packet(
        port(1U), frame(mac(1U), broadcast).serialize(), now);
    const bool vlan_isolated = isolated.disposition == asic::PacketDisposition::flooded &&
                               isolated.output_ports ==
                                   std::vector<routing::PortId>{port(2U)};
    event("vlan-isolation", vlan_isolated, "VLAN 10 flood excludes VLAN 20 port");
    static_cast<void>(device.dequeue_packet(port(2U)));

    static_cast<void>(device.process_packet(
        port(2U), frame(mac(2U), broadcast).serialize(), now));
    static_cast<void>(device.dequeue_packet(port(1U)));
    const auto learned = device.process_packet(
        port(1U), frame(mac(1U), mac(2U)).serialize(), now);
    const bool mac_learned = learned.disposition == asic::PacketDisposition::switched &&
                             learned.output_ports ==
                                 std::vector<routing::PortId>{port(2U)};
    event("mac-learning", mac_learned, "known unicast selects learned port 2");
    static_cast<void>(device.dequeue_packet(port(2U)));

    const auto prefix = network::Ipv4Prefix::create(ip(0xCB007100U), 24U).value();
    const auto route = routing::RouteEntry::create(
                           prefix, ip(0xC0A81402U), port(3U))
                           .value();
    const bool route_configured =
        device.add_or_replace_route(route) == asic::AsicStatus::success &&
        device.add_or_replace_neighbor(ip(0xC0A81402U), mac(3U), now) ==
            asic::AsicStatus::success;
    const auto packet = network::Ipv4Packet::create(
                            ip(0xC0A80A01U), ip(0xCB007109U),
                            network::IpProtocol::udp, {0x12U, 0x34U}, 64U)
                            .value();
    const auto routed = device.process_packet(
        port(1U), frame(mac(1U), mac(0xFEU), packet.serialize()).serialize(), now);
    const bool routing_passed = route_configured &&
                                routed.disposition == asic::PacketDisposition::routed &&
                                routed.output_ports ==
                                    std::vector<routing::PortId>{port(3U)};
    event("ipv4-routing", routing_passed,
          "longest-prefix route rewrites and emits on port 3");
    static_cast<void>(device.dequeue_packet(port(3U)));

    const auto queued = device.process_packet(
        port(1U), frame(mac(1U), mac(2U)).serialize(), now);
    const auto congested = device.process_packet(
        port(1U), frame(mac(1U), mac(2U)).serialize(), now);
    const bool congestion_passed =
        queued.disposition == asic::PacketDisposition::switched &&
        congested.drop_reason == asic::PacketDropReason::queue_congestion;
    event("congestion", congestion_passed,
          "capacity-one egress queue drops the second packet");
    static_cast<void>(device.dequeue_packet(port(2U)));

    asic::FaultInjectionConfig failed_port;
    failed_port.failed_ports.insert(port(1U));
    const bool fault_configured =
        device.configure_faults(failed_port) == asic::AsicStatus::success;
    const auto failed = device.process_packet(
        port(1U), frame(mac(1U), mac(2U)).serialize(), now);
    const bool failure_passed = fault_configured &&
                                failed.drop_reason ==
                                    asic::PacketDropReason::injected_fault;
    event("port-failure", failure_passed,
          "injected ingress failure drops traffic deterministically");

    const bool faults_cleared =
        device.configure_faults({}) == asic::AsicStatus::success;
    const auto recovered = device.process_packet(
        port(1U), frame(mac(1U), mac(2U)).serialize(), now);
    const bool recovery_passed = faults_cleared &&
                                 recovered.disposition ==
                                     asic::PacketDisposition::switched;
    event("recovery", recovery_passed,
          "clearing the fault restores learned unicast forwarding");
    static_cast<void>(device.dequeue_packet(port(2U)));

    const bool passed = vlan_isolated && mac_learned && routing_passed &&
                        congestion_passed && failure_passed && recovery_passed;
    const auto counters = device.counters();
    std::cout << "{\"scenario\":\"interview-demo\",\"passed\":"
              << (passed ? "true" : "false")
              << ",\"ingress_packets\":"
              << counters.get(asic::TrafficCounter::ingress_packets)
              << ",\"queue_drops\":"
              << counters.get(asic::TrafficCounter::queue_drops)
              << ",\"fault_drops\":"
              << counters.get(asic::TrafficCounter::fault_drops) << "}\n";
    return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
