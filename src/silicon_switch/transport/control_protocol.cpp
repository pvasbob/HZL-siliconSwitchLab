#include "silicon_switch/transport/control_protocol.hpp"
#include "silicon_switch/network/byte_order.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>

namespace silicon_switch::transport { namespace {
constexpr std::uint32_t magic=0x53534350U;
bool known(const ControlMessageType type){
    switch(type){
        case ControlMessageType::create_port:case ControlMessageType::remove_port:
        case ControlMessageType::set_port_state:case ControlMessageType::create_vlan:
        case ControlMessageType::remove_vlan:case ControlMessageType::add_vlan_member:
        case ControlMessageType::set_router_interface:case ControlMessageType::add_or_replace_route:
        case ControlMessageType::remove_route:case ControlMessageType::add_or_replace_neighbor:
        case ControlMessageType::remove_neighbor:case ControlMessageType::configure_faults:
        case ControlMessageType::query_counters:case ControlMessageType::response:return true;
    } return false;
}
}

std::optional<ControlMessage> ControlMessage::create(const ControlMessageType type,
    const std::uint32_t request_id,std::vector<std::uint8_t> payload){
    if(!known(type)||payload.size()>maximum_payload_size)return std::nullopt;
    return ControlMessage{type,request_id,std::move(payload)};
}

std::variant<ControlMessage,ControlParseError> ControlMessage::parse(const network::ByteView bytes){
    if(bytes.size()<header_size)return ControlParseError::truncated;
    const auto wire_magic=network::wire::read_big_endian<std::uint32_t>(bytes,0U).value();
    const auto version=network::wire::read_big_endian<std::uint16_t>(bytes,4U).value();
    const auto raw_type=network::wire::read_big_endian<std::uint16_t>(bytes,6U).value();
    const auto request=network::wire::read_big_endian<std::uint32_t>(bytes,8U).value();
    const auto length=network::wire::read_big_endian<std::uint32_t>(bytes,12U).value();
    if(wire_magic!=magic)return ControlParseError::invalid_magic;
    if(version!=current_version)return ControlParseError::unsupported_version;
    const auto type=static_cast<ControlMessageType>(raw_type);
    if(!known(type))return ControlParseError::unknown_message_type;
    if(length>maximum_payload_size)return ControlParseError::payload_too_large;
    if(bytes.size()!=header_size+length)return ControlParseError::length_mismatch;
    std::vector<std::uint8_t> payload(bytes.begin()+static_cast<std::ptrdiff_t>(header_size),bytes.end());
    return ControlMessage{type,request,std::move(payload)};
}

std::vector<std::uint8_t> ControlMessage::serialize() const{
    std::vector<std::uint8_t> bytes(header_size+payload_.size());
    network::MutableByteView output{bytes};
    static_cast<void>(network::wire::write_big_endian<std::uint32_t>(magic,output,0U));
    static_cast<void>(network::wire::write_big_endian<std::uint16_t>(version_,output,4U));
    static_cast<void>(network::wire::write_big_endian<std::uint16_t>(static_cast<std::uint16_t>(type_),output,6U));
    static_cast<void>(network::wire::write_big_endian<std::uint32_t>(request_id_,output,8U));
    static_cast<void>(network::wire::write_big_endian<std::uint32_t>(static_cast<std::uint32_t>(payload_.size()),output,12U));
    std::copy(payload_.begin(),payload_.end(),bytes.begin()+static_cast<std::ptrdiff_t>(header_size));
    return bytes;
}

ControlMessage::ControlMessage(const ControlMessageType type,const std::uint32_t request,
    std::vector<std::uint8_t> payload):type_{type},request_id_{request},payload_{std::move(payload)}{}

}  // namespace silicon_switch::transport
