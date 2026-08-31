#include "transport/transport_test.hpp"
#include "silicon_switch/transport/control_protocol.hpp"
#include "silicon_switch/transport/socket_handle.hpp"
#include "silicon_switch/transport/tcp.hpp"
#include "test_support.hpp"
#include <cstdint>
#include <thread>
#include <variant>
#include <vector>

namespace silicon_switch::test {
void run_transport_tests(TestSuite& suite){
    transport::SocketRuntime runtime;
    auto server_result=transport::TcpServer::listen_ipv4("127.0.0.1",0U);
    suite.expect_true(std::holds_alternative<transport::TcpServer>(server_result),
                      "listen on loopback TCP socket");
    if(!std::holds_alternative<transport::TcpServer>(server_result))return;
    auto server=std::get<transport::TcpServer>(std::move(server_result));
    suite.expect_true(server.local_port()!=0U,"discover ephemeral TCP server port");
    bool server_received=false;
    std::thread worker{[&server,&server_received]{
        auto accepted=server.accept();
        if(!std::holds_alternative<transport::TcpConnection>(accepted))return;
        auto connection=std::get<transport::TcpConnection>(std::move(accepted));
        auto incoming=connection.receive_frame(1024U);
        if(!std::holds_alternative<std::vector<std::uint8_t>>(incoming))return;
        server_received=std::get<std::vector<std::uint8_t>>(incoming)==std::vector<std::uint8_t>{1U,2U,3U};
        static_cast<void>(connection.send_frame(std::vector<std::uint8_t>{4U,5U}));
        static_cast<void>(connection.shutdown());
    }};
    auto client_result=transport::TcpConnection::connect_ipv4("127.0.0.1",server.local_port());
    suite.expect_true(std::holds_alternative<transport::TcpConnection>(client_result),
                      "connect loopback TCP client");
    if(std::holds_alternative<transport::TcpConnection>(client_result)){
        auto client=std::get<transport::TcpConnection>(std::move(client_result));
        suite.expect_false(client.send_frame(std::vector<std::uint8_t>{1U,2U,3U}).has_value(),
                           "send length-prefixed TCP frame");
        auto response=client.receive_frame(1024U);
        suite.expect_true(std::holds_alternative<std::vector<std::uint8_t>>(response),
                          "receive length-prefixed TCP frame");
        if(std::holds_alternative<std::vector<std::uint8_t>>(response))
            suite.expect_equal(std::get<std::vector<std::uint8_t>>(response),
                               std::vector<std::uint8_t>{4U,5U},"preserve TCP frame payload");
    }
    worker.join();
    suite.expect_true(server_received,"TCP server receives complete framed payload");

    const auto message=transport::ControlMessage::create(
        transport::ControlMessageType::add_or_replace_route,0x12345678U,{9U,8U,7U});
    suite.expect_true(message.has_value(),"create versioned control message");
    const auto bytes=message->serialize();
    const auto parsed=transport::ControlMessage::parse(bytes);
    suite.expect_true(std::holds_alternative<transport::ControlMessage>(parsed),
                      "parse versioned control message");
    if(std::holds_alternative<transport::ControlMessage>(parsed)){
        const auto& value=std::get<transport::ControlMessage>(parsed);
        suite.expect_equal(value.version(),transport::ControlMessage::current_version,
                           "preserve control protocol version");
        suite.expect_equal(value.type(),transport::ControlMessageType::add_or_replace_route,
                           "preserve control command type");
        suite.expect_equal(value.request_id(),std::uint32_t{0x12345678U},
                           "preserve control request identifier");
        suite.expect_equal(value.payload(),std::vector<std::uint8_t>{9U,8U,7U},
                           "preserve control command payload");
    }
    suite.expect_equal(std::get<transport::ControlParseError>(
        transport::ControlMessage::parse(std::vector<std::uint8_t>{1U,2U})),
        transport::ControlParseError::truncated,"reject truncated control message");
    auto invalid_magic=bytes;invalid_magic[0U]=0U;
    suite.expect_equal(std::get<transport::ControlParseError>(transport::ControlMessage::parse(invalid_magic)),
                       transport::ControlParseError::invalid_magic,"reject control message magic");
    auto invalid_version=bytes;invalid_version[5U]=2U;
    suite.expect_equal(std::get<transport::ControlParseError>(transport::ControlMessage::parse(invalid_version)),
                       transport::ControlParseError::unsupported_version,"reject unsupported control version");
    auto unknown_type=bytes;unknown_type[6U]=0x7FU;unknown_type[7U]=0xFFU;
    suite.expect_equal(std::get<transport::ControlParseError>(transport::ControlMessage::parse(unknown_type)),
                       transport::ControlParseError::unknown_message_type,"reject unknown control command");
    auto wrong_length=bytes;wrong_length[15U]=4U;
    suite.expect_equal(std::get<transport::ControlParseError>(transport::ControlMessage::parse(wrong_length)),
                       transport::ControlParseError::length_mismatch,"reject mismatched control payload length");
    suite.expect_true(transport::ControlMessage::create(transport::ControlMessageType::query_counters,7U)
                          .has_value(),"create empty counter query command");
}
}
