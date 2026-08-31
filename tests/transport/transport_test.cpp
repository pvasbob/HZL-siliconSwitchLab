#include "transport/transport_test.hpp"
#include "silicon_switch/transport/control_protocol.hpp"
#include "silicon_switch/transport/client_synchronizer.hpp"
#include "silicon_switch/transport/socket_handle.hpp"
#include "silicon_switch/transport/state_update_protocol.hpp"
#include "silicon_switch/transport/tcp.hpp"
#include "silicon_switch/transport/udp.hpp"
#include "test_support.hpp"
#include <chrono>
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

    const auto snapshot = transport::StateUpdate::create(
        transport::StateUpdateType::snapshot, 42U, 1U, 1'000U, 0U, 7U,
        {10U, 20U, 30U});
    suite.expect_true(snapshot.has_value(), "create UDP state snapshot");
    const auto snapshot_bytes = snapshot->serialize();
    const auto parsed_snapshot = transport::StateUpdate::parse(snapshot_bytes);
    suite.expect_true(std::holds_alternative<transport::StateUpdate>(parsed_snapshot),
                      "parse UDP state snapshot");
    if (std::holds_alternative<transport::StateUpdate>(parsed_snapshot)) {
        const auto& update = std::get<transport::StateUpdate>(parsed_snapshot);
        suite.expect_equal(update.type(), transport::StateUpdateType::snapshot,
                           "preserve state-update type");
        suite.expect_equal(update.session_id(), std::uint64_t{42U},
                           "preserve state-update session");
        suite.expect_equal(update.sequence(), std::uint64_t{1U},
                           "preserve state-update sequence");
        suite.expect_equal(update.timestamp_microseconds(), std::uint64_t{1'000U},
                           "preserve state-update timestamp");
        suite.expect_equal(update.revision(), std::uint64_t{7U},
                           "preserve state revision");
        suite.expect_equal(update.payload(), std::vector<std::uint8_t>({10U, 20U, 30U}),
                           "preserve state-update payload");
    }
    auto invalid_state_magic = snapshot_bytes;
    invalid_state_magic[0U] = 0U;
    suite.expect_equal(
        std::get<transport::StateUpdateParseError>(
            transport::StateUpdate::parse(invalid_state_magic)),
        transport::StateUpdateParseError::invalid_magic,
        "reject invalid state-update magic");
    auto invalid_state_length = snapshot_bytes;
    invalid_state_length[51U] = 4U;
    suite.expect_equal(
        std::get<transport::StateUpdateParseError>(
            transport::StateUpdate::parse(invalid_state_length)),
        transport::StateUpdateParseError::length_mismatch,
        "reject invalid state-update length");
    suite.expect_false(
        transport::StateUpdate::create(transport::StateUpdateType::delta, 42U, 2U,
                                       2'000U, 8U, 8U).has_value(),
        "reject non-advancing delta revision");

    auto receiver_result = transport::UdpSocket::bind_ipv4("127.0.0.1", 0U);
    auto sender_result = transport::UdpSocket::bind_ipv4("127.0.0.1", 0U);
    suite.expect_true(std::holds_alternative<transport::UdpSocket>(receiver_result),
                      "bind UDP state receiver");
    suite.expect_true(std::holds_alternative<transport::UdpSocket>(sender_result),
                      "bind UDP state sender");
    if (std::holds_alternative<transport::UdpSocket>(receiver_result) &&
        std::holds_alternative<transport::UdpSocket>(sender_result)) {
        auto receiver = std::get<transport::UdpSocket>(std::move(receiver_result));
        auto sender = std::get<transport::UdpSocket>(std::move(sender_result));
        const auto sent = sender.send_to_ipv4("127.0.0.1", receiver.local_port(),
                                              snapshot_bytes);
        suite.expect_true(std::holds_alternative<std::size_t>(sent),
                          "send UDP state datagram");
        const auto received = receiver.receive(transport::StateUpdate::header_size +
                                               transport::StateUpdate::maximum_payload_size);
        suite.expect_true(std::holds_alternative<transport::UdpDatagram>(received),
                          "receive UDP state datagram");
        if (std::holds_alternative<transport::UdpDatagram>(received)) {
            suite.expect_equal(std::get<transport::UdpDatagram>(received).payload,
                               snapshot_bytes, "preserve UDP datagram payload");
        }
    }

    transport::ClientSynchronizer synchronizer;
    const auto initial_time = transport::ClientSynchronizer::Clock::time_point{};
    suite.expect_true(synchronizer.resync_required(),
                      "new synchronization client requires snapshot");
    suite.expect_equal(synchronizer.consume(*snapshot, initial_time),
                       transport::SynchronizationResult::applied_snapshot,
                       "apply initial synchronization snapshot");
    suite.expect_true(synchronizer.synchronized(), "client becomes synchronized");
    const auto delta = transport::StateUpdate::create(
        transport::StateUpdateType::delta, 42U, 2U, 2'000U, 7U, 8U, {40U});
    suite.expect_equal(synchronizer.consume(*delta, initial_time + std::chrono::seconds{1}),
                       transport::SynchronizationResult::applied_delta,
                       "apply contiguous state delta");
    suite.expect_equal(synchronizer.revision(), std::uint64_t{8U},
                       "advance synchronized revision");
    suite.expect_equal(synchronizer.consume(*delta),
                       transport::SynchronizationResult::ignored_duplicate,
                       "ignore duplicate state update");
    const auto gap = transport::StateUpdate::create(
        transport::StateUpdateType::delta, 42U, 4U, 4'000U, 8U, 9U);
    suite.expect_equal(synchronizer.consume(*gap),
                       transport::SynchronizationResult::sequence_gap,
                       "detect lost state update");
    suite.expect_true(synchronizer.resync_required(),
                      "sequence gap requests resynchronization");
    const auto new_session_delta = transport::StateUpdate::create(
        transport::StateUpdateType::delta, 99U, 1U, 5'000U, 1U, 2U);
    suite.expect_equal(synchronizer.consume(*new_session_delta),
                       transport::SynchronizationResult::needs_snapshot,
                       "require snapshot after server restart");
    const auto new_snapshot = transport::StateUpdate::create(
        transport::StateUpdateType::snapshot, 99U, 2U, 6'000U, 0U, 12U);
    suite.expect_equal(synchronizer.consume(*new_snapshot, initial_time),
                       transport::SynchronizationResult::applied_snapshot,
                       "resynchronize from replacement snapshot");
    suite.expect_false(synchronizer.disconnected(
                           initial_time + std::chrono::milliseconds{500},
                           std::chrono::seconds{1}),
                       "client remains connected before timeout");
    suite.expect_true(synchronizer.disconnected(
                          initial_time + std::chrono::seconds{2},
                          std::chrono::seconds{1}),
                      "detect synchronization timeout");
}
}
