#include "asic/bounded_queue_test.hpp"
#include "asic/fault_injector_test.hpp"
#include "asic/traffic_statistics_test.hpp"
#include "network/arp_packet_test.hpp"
#include "network/byte_order_test.hpp"
#include "network/ethernet_frame_test.hpp"
#include "network/internet_checksum_test.hpp"
#include "network/ipv4_address_test.hpp"
#include "network/ipv4_packet_test.hpp"
#include "network/ipv4_prefix_test.hpp"
#include "network/mac_address_test.hpp"
#include "network/vlan_test.hpp"
#include "routing/arp_cache_test.hpp"
#include "routing/ipv4_forwarding_engine_test.hpp"
#include "routing/ipv4_forwarding_result_test.hpp"
#include "routing/ipv4_route_table_test.hpp"
#include "routing/ipv4_ttl_test.hpp"
#include "routing/l3_ethernet_encapsulation_test.hpp"
#include "silicon_switch/version.hpp"
#include "switching/mac_table_test.hpp"
#include "switching/l2_forwarding_test.hpp"
#include "switching/l2_l3_pipeline_test.hpp"
#include "switching/virtual_port_test.hpp"
#include "switching/vlan_ingress_test.hpp"
#include "switching/vlan_port_config_test.hpp"
#include "test_support.hpp"

#include <string_view>

int main() {
    silicon_switch::test::TestSuite suite;

    suite.expect_equal(
        silicon_switch::version(), std::string_view{"0.1.0"}, "library version");
    silicon_switch::test::run_bounded_queue_tests(suite);
    silicon_switch::test::run_fault_injector_tests(suite);
    silicon_switch::test::run_traffic_statistics_tests(suite);
    silicon_switch::test::run_arp_packet_tests(suite);
    silicon_switch::test::run_byte_order_tests(suite);
    silicon_switch::test::run_ethernet_frame_tests(suite);
    silicon_switch::test::run_internet_checksum_tests(suite);
    silicon_switch::test::run_mac_address_tests(suite);
    silicon_switch::test::run_ipv4_address_tests(suite);
    silicon_switch::test::run_ipv4_packet_tests(suite);
    silicon_switch::test::run_ipv4_prefix_tests(suite);
    silicon_switch::test::run_vlan_tests(suite);
    silicon_switch::test::run_arp_cache_tests(suite);
    silicon_switch::test::run_ipv4_forwarding_engine_tests(suite);
    silicon_switch::test::run_ipv4_forwarding_result_tests(suite);
    silicon_switch::test::run_ipv4_route_table_tests(suite);
    silicon_switch::test::run_ipv4_ttl_tests(suite);
    silicon_switch::test::run_l3_ethernet_encapsulation_tests(suite);
    silicon_switch::test::run_mac_table_tests(suite);
    silicon_switch::test::run_l2_forwarding_tests(suite);
    silicon_switch::test::run_l2_l3_pipeline_tests(suite);
    silicon_switch::test::run_virtual_port_tests(suite);
    silicon_switch::test::run_vlan_port_config_tests(suite);
    silicon_switch::test::run_vlan_ingress_tests(suite);

    return suite.exit_code();
}
#include "asic/bounded_queue_test.hpp"
#include "asic/fault_injector_test.hpp"
#include "asic/traffic_statistics_test.hpp"
