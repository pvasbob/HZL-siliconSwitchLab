#include "silicon_switch/transport/tcp.hpp"

#include "silicon_switch/network/byte_order.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace silicon_switch::transport { namespace {
TcpError system_error(){return {TcpErrorCode::system_error,last_socket_error()};}
TcpError disconnected(){return {TcpErrorCode::disconnected,{0,"peer disconnected"}};}
bool interrupted(){
#ifdef _WIN32
 return WSAGetLastError()==WSAEINTR;
#else
 return errno==EINTR;
#endif
}
int send_flags(){
#ifdef MSG_NOSIGNAL
 return MSG_NOSIGNAL;
#else
 return 0;
#endif
}
} 

TcpConnection::ConnectResult TcpConnection::connect_ipv4(const std::string& address,const std::uint16_t port){
    SocketHandle socket{::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)};
    if(!socket.valid()) return system_error();
    sockaddr_in endpoint{};endpoint.sin_family=AF_INET;endpoint.sin_port=htons(port);
    if(inet_pton(AF_INET,address.c_str(),&endpoint.sin_addr)!=1)
        return TcpError{TcpErrorCode::system_error,{0,"invalid IPv4 address"}};
    if(::connect(socket.native_handle(),reinterpret_cast<const sockaddr*>(&endpoint),sizeof(endpoint))!=0)
        return system_error();
    return TcpConnection{std::move(socket)};
}

std::optional<TcpError> TcpConnection::send_all(const network::ByteView bytes){
    std::size_t sent=0U;
    while(sent<bytes.size()){
        const auto remaining=bytes.size()-sent;
#ifdef _WIN32
        const int chunk=::send(socket_.native_handle(),reinterpret_cast<const char*>(bytes.begin()+sent),
            static_cast<int>(std::min(remaining,static_cast<std::size_t>(std::numeric_limits<int>::max()))),send_flags());
#else
        const auto chunk=::send(socket_.native_handle(),bytes.begin()+sent,remaining,send_flags());
#endif
        if(chunk==0) return disconnected();
        if(chunk<0){if(interrupted())continue;return system_error();}
        sent+=static_cast<std::size_t>(chunk);
    }
    return std::nullopt;
}

TcpConnection::ReceiveResult TcpConnection::receive_exact(const std::size_t size){
    std::vector<std::uint8_t> bytes(size);std::size_t received=0U;
    while(received<size){
        const auto remaining=size-received;
#ifdef _WIN32
        const int chunk=::recv(socket_.native_handle(),reinterpret_cast<char*>(bytes.data()+received),
            static_cast<int>(std::min(remaining,static_cast<std::size_t>(std::numeric_limits<int>::max()))),0);
#else
        const auto chunk=::recv(socket_.native_handle(),bytes.data()+received,remaining,0);
#endif
        if(chunk==0)return disconnected();
        if(chunk<0){if(interrupted())continue;return system_error();}
        received+=static_cast<std::size_t>(chunk);
    }
    return bytes;
}

std::optional<TcpError> TcpConnection::send_frame(const network::ByteView payload){
    if(payload.size()>std::numeric_limits<std::uint32_t>::max())
        return TcpError{TcpErrorCode::message_too_large,{0,"frame exceeds 32-bit length"}};
    std::array<std::uint8_t,4U> header{};
    static_cast<void>(network::wire::write_big_endian<std::uint32_t>(
        static_cast<std::uint32_t>(payload.size()),network::MutableByteView{header}));
    if(auto error=send_all(header))return error;
    return send_all(payload);
}

TcpConnection::ReceiveResult TcpConnection::receive_frame(const std::size_t maximum_size){
    auto header_result=receive_exact(4U);
    if(const auto* error=std::get_if<TcpError>(&header_result))return *error;
    const auto& header=std::get<std::vector<std::uint8_t>>(header_result);
    const auto length=network::wire::read_big_endian<std::uint32_t>(header).value();
    if(length>maximum_size)return TcpError{TcpErrorCode::message_too_large,{0,"received frame too large"}};
    return receive_exact(length);
}

TcpServer::ListenResult TcpServer::listen_ipv4(const std::string& address,const std::uint16_t port,const int backlog){
    SocketHandle socket{::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)};
    if(!socket.valid())return system_error();
    int reuse=1;
#ifdef _WIN32
    setsockopt(socket.native_handle(),SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&reuse),sizeof(reuse));
#else
    setsockopt(socket.native_handle(),SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
#endif
    sockaddr_in endpoint{};endpoint.sin_family=AF_INET;endpoint.sin_port=htons(port);
    if(inet_pton(AF_INET,address.c_str(),&endpoint.sin_addr)!=1)
        return TcpError{TcpErrorCode::system_error,{0,"invalid IPv4 address"}};
    if(::bind(socket.native_handle(),reinterpret_cast<const sockaddr*>(&endpoint),sizeof(endpoint))!=0 ||
       ::listen(socket.native_handle(),backlog)!=0)return system_error();
    return TcpServer{std::move(socket)};
}

TcpServer::AcceptResult TcpServer::accept(){
    for(;;){NativeSocket accepted=::accept(socket_.native_handle(),nullptr,nullptr);
        if(accepted!=invalid_socket)return TcpConnection{SocketHandle{accepted}};
        if(!interrupted())return system_error();}
}

std::uint16_t TcpServer::local_port() const{
    sockaddr_in endpoint{};
#ifdef _WIN32
    int size=sizeof(endpoint);
#else
    socklen_t size=sizeof(endpoint);
#endif
    if(getsockname(socket_.native_handle(),reinterpret_cast<sockaddr*>(&endpoint),&size)!=0)return 0U;
    return ntohs(endpoint.sin_port);
}

}  // namespace silicon_switch::transport
