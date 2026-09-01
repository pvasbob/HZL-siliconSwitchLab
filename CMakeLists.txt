cmake_minimum_required(VERSION 3.20)

project(
    SiliconSwitchLab
    VERSION 0.1.0
    DESCRIPTION "An educational software model of a network switch ASIC"
    LANGUAGES CXX
)

find_package(Threads REQUIRED)

option(SILICON_SWITCH_ENABLE_CUDA "Build the optional CUDA simulation backend" ON)
set(SILICON_SWITCH_CUDA_SOURCE
    src/silicon_switch/simulation/cuda_backend_stub.cpp
)
if(SILICON_SWITCH_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(CMAKE_CUDA_COMPILER)
        enable_language(CUDA)
        find_package(CUDAToolkit REQUIRED)
        set(SILICON_SWITCH_CUDA_SOURCE
            src/silicon_switch/simulation/cuda_backend.cu
        )
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(silicon_switch
    src/silicon_switch/asic/fault_injector.cpp
    src/silicon_switch/asic/mock_switch_asic.cpp
    src/silicon_switch/asic/programmable_switch.cpp
    src/silicon_switch/asic/software_asic.cpp
    src/silicon_switch/asic/traffic_statistics.cpp
    src/silicon_switch/discovery/discovery_protocol.cpp
    src/silicon_switch/discovery/discovery_service.cpp
    src/silicon_switch/discovery/lab_configuration.cpp
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
    src/silicon_switch/simulation/authoritative_server.cpp
    src/silicon_switch/simulation/cpu_backend.cpp
    src/silicon_switch/simulation/runtime_selection.cpp
    ${SILICON_SWITCH_CUDA_SOURCE}
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
    src/silicon_switch/visualization/application.cpp
    src/silicon_switch/visualization/observer_client.cpp
    src/silicon_switch/visualization/topology.cpp
    src/silicon_switch/visualization/topology_renderer.cpp
    src/silicon_switch/version.cpp
)

target_include_directories(silicon_switch
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(CMAKE_CUDA_COMPILER AND SILICON_SWITCH_ENABLE_CUDA)
    set_target_properties(silicon_switch PROPERTIES
        CUDA_ARCHITECTURES 86
        CUDA_STANDARD 17
        CUDA_STANDARD_REQUIRED ON
    )
    target_link_libraries(silicon_switch PUBLIC CUDA::cudart)
endif()

add_executable(silicon_switch_cli
    apps/switch_cli/main.cpp
)

add_executable(silicon_switch_benchmark
    apps/benchmark/main.cpp
)
target_link_libraries(silicon_switch_benchmark PRIVATE silicon_switch)

add_executable(silicon_switch_discovery
    apps/discovery/main.cpp
)
target_link_libraries(silicon_switch_discovery PRIVATE silicon_switch)

add_executable(silicon_switch_server
    apps/simulation_server/main.cpp
)

add_executable(silicon_switch_observer_node
    apps/integration_observer/main.cpp
)
target_link_libraries(silicon_switch_observer_node PRIVATE silicon_switch)
target_link_libraries(silicon_switch_server
    PRIVATE
        silicon_switch
        Threads::Threads
)

find_package(OpenGL QUIET)
find_package(glfw3 CONFIG QUIET)
if(OpenGL_FOUND AND TARGET glfw)
    add_executable(silicon_switch_observer
        apps/observer/main.cpp
    )
    target_link_libraries(silicon_switch_observer
        PRIVATE
            silicon_switch
            OpenGL::GL
            glfw
    )
endif()

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
    tests/discovery/discovery_test.cpp
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
    tests/simulation/simulation_test.cpp
    tests/switching/mac_table_test.cpp
    tests/switching/l2_forwarding_test.cpp
    tests/switching/l2_l3_pipeline_test.cpp
    tests/switching/virtual_port_test.cpp
    tests/switching/vlan_ingress_test.cpp
    tests/switching/vlan_port_config_test.cpp
    tests/transport/transport_test.cpp
    tests/visualization/visualization_test.cpp
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
        target_compile_options(${target_name} PRIVATE
            $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic;-Wconversion;-Wsign-conversion>
        )
    endif()
endfunction()

enable_strict_warnings(silicon_switch)
enable_strict_warnings(silicon_switch_cli)
enable_strict_warnings(silicon_switch_benchmark)
enable_strict_warnings(silicon_switch_discovery)
enable_strict_warnings(silicon_switch_server)
enable_strict_warnings(silicon_switch_observer_node)
enable_strict_warnings(silicon_switch_tests)
if(TARGET silicon_switch_observer)
    enable_strict_warnings(silicon_switch_observer)
endif()

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
        add_test(
            NAME silicon_switch_four_process_integration
            COMMAND ${Python3_EXECUTABLE}
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/four_process_scenario.py"
                    --server $<TARGET_FILE:silicon_switch_server>
                    --observer $<TARGET_FILE:silicon_switch_observer_node>
        )
        set_tests_properties(silicon_switch_four_process_integration PROPERTIES
            TIMEOUT 20
        )
        add_test(
            NAME silicon_switch_benchmark_smoke
            COMMAND ${Python3_EXECUTABLE}
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integration/benchmark_smoke.py"
                    --benchmark $<TARGET_FILE:silicon_switch_benchmark>
        )
        set_tests_properties(silicon_switch_benchmark_smoke PROPERTIES TIMEOUT 20)
    endif()
endif()
