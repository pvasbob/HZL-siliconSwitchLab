#include "routing/ipv4_forwarding_result_test.hpp"

#include "silicon_switch/network/ip_protocol.hpp"
#include "silicon_switch/network/ipv4_address.hpp"
#include "silicon_switch/network/ipv4_packet.hpp"
#include "silicon_switch/routing/ipv4_forwarding_result.hpp"
#include "silicon_switch/routing/port_id.hpp"
#include "test_support.hpp"

#include <cstdint>
#include <variant>
#include <vector>

namespace silicon_switch::test {

void run_ipv4_forwarding_result_tests(TestSuite& suite) {
    const auto packet = network::Ipv4Packet::create(
        network::Ipv4Address{0xC0A8010AU},
        network::Ipv4Address{0xCB007109U},
        network::IpProtocol::udp,
        std::vector<std::uint8_t>{0x12U, 0x34U},
        63U);
    const auto output_port = routing::PortId::create(7U);

    suite.expect_true(packet.has_value(), "create packet for forwarding result");
    suite.expect_true(output_port.has_value(),
                      "create output port for forwarding result");
    if (!packet.has_value() || !output_port.has_value()) {
        return;
    }

    const network::Ipv4Address next_hop{0xC0A80101U};
    const routing::Ipv4ForwardingResult forwarded =
        routing::ForwardedIpv4Packet{*packet, *output_port, next_hop};
    suite.expect_true(
        std::holds_alternative<routing::ForwardedIpv4Packet>(forwarded),
        "represent forwarded IPv4 outcome");
    const auto& forwarding =
        std::get<routing::ForwardedIpv4Packet>(forwarded);
    suite.expect_equal(forwarding.packet(), *packet,
                       "forwarded outcome owns resulting packet");
    suite.expect_equal(forwarding.output_port(), *output_port,
                       "forwarded outcome identifies output port");
    suite.expect_equal(forwarding.next_hop(), next_hop,
                       "forwarded outcome identifies next hop");

    const routing::Ipv4ForwardingResult ttl_drop =
        routing::DroppedIpv4Packet{
            routing::Ipv4DropReason::time_to_live_expired};
    suite.expect_true(
        std::holds_alternative<routing::DroppedIpv4Packet>(ttl_drop),
        "represent dropped IPv4 outcome");
    suite.expect_equal(
        std::get<routing::DroppedIpv4Packet>(ttl_drop).reason(),
        routing::Ipv4DropReason::time_to_live_expired,
        "report TTL-expired drop reason");

    const routing::Ipv4ForwardingResult route_drop =
        routing::DroppedIpv4Packet{routing::Ipv4DropReason::route_not_found};
    suite.expect_equal(
        std::get<routing::DroppedIpv4Packet>(route_drop).reason(),
        routing::Ipv4DropReason::route_not_found,
        "report route-miss drop reason");
}

}  // namespace silicon_switch::test
