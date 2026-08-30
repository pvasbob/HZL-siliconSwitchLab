cmake_minimum_required(VERSION 3.20)

project(
    SiliconSwitchLab
    VERSION 0.1.0
    DESCRIPTION "An educational software model of a network switch ASIC"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(silicon_switch
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
    tests/test_main.cpp
)

target_include_directories(silicon_switch_tests
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests"
)

target_link_libraries(silicon_switch_tests
    PRIVATE
        silicon_switch
)

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
endif()
