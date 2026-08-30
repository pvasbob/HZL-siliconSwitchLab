#include "network/arp_packet_test.hpp"
#include "network/byte_order_test.hpp"
#include "network/ethernet_frame_test.hpp"
#include "network/internet_checksum_test.hpp"
#include "network/ipv4_address_test.hpp"
#include "network/ipv4_packet_test.hpp"
#include "network/ipv4_prefix_test.hpp"
#include "network/mac_address_test.hpp"
#include "network/vlan_test.hpp"
#include "routing/ipv4_forwarding_engine_test.hpp"
#include "routing/ipv4_forwarding_result_test.hpp"
#include "routing/ipv4_route_table_test.hpp"
#include "routing/ipv4_ttl_test.hpp"
#include "silicon_switch/version.hpp"
#include "test_support.hpp"

#include <string_view>

int main() {
    silicon_switch::test::TestSuite suite;

    suite.expect_equal(
        silicon_switch::version(), std::string_view{"0.1.0"}, "library version");
    silicon_switch::test::run_arp_packet_tests(suite);
    silicon_switch::test::run_byte_order_tests(suite);
    silicon_switch::test::run_ethernet_frame_tests(suite);
    silicon_switch::test::run_internet_checksum_tests(suite);
    silicon_switch::test::run_mac_address_tests(suite);
    silicon_switch::test::run_ipv4_address_tests(suite);
    silicon_switch::test::run_ipv4_packet_tests(suite);
    silicon_switch::test::run_ipv4_prefix_tests(suite);
    silicon_switch::test::run_vlan_tests(suite);
    silicon_switch::test::run_ipv4_forwarding_engine_tests(suite);
    silicon_switch::test::run_ipv4_forwarding_result_tests(suite);
    silicon_switch::test::run_ipv4_route_table_tests(suite);
    silicon_switch::test::run_ipv4_ttl_tests(suite);

    return suite.exit_code();
}
