cmake_minimum_required(VERSION 3.20)

project(
    SiliconSwitchLab
    VERSION 0.1.0
    DESCRIPTION "An educational software model of a network switch ASIC"
    LANGUAGES CXX
)

find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(silicon_switch
    src/silicon_switch/asic/fault_injector.cpp
    src/silicon_switch/asic/mock_switch_asic.cpp
    src/silicon_switch/asic/programmable_switch.cpp
    src/silicon_switch/asic/software_asic.cpp
    src/silicon_switch/asic/traffic_statistics.cpp
    src/silicon_switch/network/arp_packet.cpp
    src/silicon_switch/network/ethernet_frame.cpp
    src/silicon_switch/network/internet_checksum.cpp
    src/silicon_switch/network/ipv4_address.cpp
    src/silicon_switch/network/ipv4_packet.cpp
    src/silicon_switch/network/ipv4_prefix.cpp
    src/silicon_switch/network/mac_address.cpp
    src/silicon_switch/routing/arp_cache.cpp
    src/silicon_switch/routing/ipv4_forwarding_engine.cpp
    src/silicon_switch/routing/ipv4_route_table.cpp
    src/silicon_switch/routing/ipv4_ttl.cpp
    src/silicon_switch/routing/l3_ethernet_encapsulation.cpp
    src/silicon_switch/switching/mac_table.cpp
    src/silicon_switch/switching/l2_forwarding.cpp
    src/silicon_switch/switching/l2_l3_pipeline.cpp
    src/silicon_switch/switching/virtual_port.cpp
    src/silicon_switch/switching/vlan_ingress.cpp
    src/silicon_switch/switching/vlan_port_config.cpp
    src/silicon_switch/transport/control_protocol.cpp
    src/silicon_switch/transport/client_synchronizer.cpp
    src/silicon_switch/transport/socket_handle.cpp
    src/silicon_switch/transport/state_update_protocol.cpp
    src/silicon_switch/transport/tcp.cpp
    src/silicon_switch/transport/udp.cpp
    src/silicon_switch/version.cpp
)

target_include_directories(silicon_switch
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

add_executable(silicon_switch_cli
    apps/switch_cli/main.cpp
)

target_link_libraries(silicon_switch_cli
    PRIVATE
        silicon_switch
)

add_executable(silicon_switch_tests
    tests/asic/bounded_queue_test.cpp
    tests/asic/fault_injector_test.cpp
    tests/asic/programmable_switch_test.cpp
    tests/asic/software_asic_test.cpp
    tests/asic/traffic_statistics_test.cpp
    tests/network/arp_packet_test.cpp
    tests/network/byte_order_test.cpp
    tests/network/ethernet_frame_test.cpp
    tests/network/internet_checksum_test.cpp
    tests/network/ipv4_address_test.cpp
    tests/network/ipv4_packet_test.cpp
    tests/network/ipv4_prefix_test.cpp
    tests/network/mac_address_test.cpp
    tests/network/vlan_test.cpp
    tests/routing/arp_cache_test.cpp
    tests/routing/ipv4_forwarding_engine_test.cpp
    tests/routing/ipv4_forwarding_result_test.cpp
    tests/routing/ipv4_route_table_test.cpp
    tests/routing/ipv4_ttl_test.cpp
    tests/routing/l3_ethernet_encapsulation_test.cpp
    tests/switching/mac_table_test.cpp
    tests/switching/l2_forwarding_test.cpp
    tests/switching/l2_l3_pipeline_test.cpp
    tests/switching/virtual_port_test.cpp
    tests/switching/vlan_ingress_test.cpp
    tests/switching/vlan_port_config_test.cpp
    tests/transport/transport_test.cpp
    tests/test_main.cpp
)

target_include_directories(silicon_switch_tests
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)

target_link_libraries(silicon_switch_tests
    PRIVATE
        silicon_switch
        Threads::Threads
)

if(WIN32)
    target_link_libraries(silicon_switch PUBLIC ws2_32)
endif()

function(enable_strict_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(
            ${target_name}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
        )
    endif()
endfunction()

enable_strict_warnings(silicon_switch)
enable_strict_warnings(silicon_switch_cli)
enable_strict_warnings(silicon_switch_tests)

include(CTest)

if(BUILD_TESTING)
    add_test(
        NAME silicon_switch_unit_tests
        COMMAND silicon_switch_tests
    )

    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(
            NAME silicon_switch_python_tests
            COMMAND ${Python3_EXECUTABLE} -m unittest discover
                    -s "${CMAKE_CURRENT_SOURCE_DIR}/tests/python"
                    -p "test_*.py"
        )
        set_tests_properties(silicon_switch_python_tests PROPERTIES
            ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/python"
        )
    endif()
endif()
