#include "silicon_switch/transport/socket_handle.hpp"

#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <cerrno>
#include <cstring>
#include <unistd.h>
#endif

namespace silicon_switch::transport {

SocketError last_socket_error() {
#ifdef _WIN32
    const int code=WSAGetLastError();
    return {code,"Winsock error " + std::to_string(code)};
#else
    const int code=errno;
    return {code,std::generic_category().message(code)};
#endif
}

SocketRuntime::SocketRuntime() {
#ifdef _WIN32
    WSADATA data{};
    const int result=WSAStartup(MAKEWORD(2,2),&data);
    if(result!=0) throw std::runtime_error{"WSAStartup failed: " + std::to_string(result)};
    initialized_=true;
#else
    initialized_=true;
#endif
}

SocketRuntime::~SocketRuntime() {
#ifdef _WIN32
    if(initialized_) WSACleanup();
#endif
}

void ensure_socket_runtime() {
    static const SocketRuntime runtime;
    static_cast<void>(runtime);
}

SocketHandle::~SocketHandle(){reset();}
SocketHandle::SocketHandle(SocketHandle&& other) noexcept:handle_{other.release()}{}
SocketHandle& SocketHandle::operator=(SocketHandle&& other) noexcept {
    if(this!=&other) reset(other.release());
    return *this;
}
NativeSocket SocketHandle::release() noexcept {const auto result=handle_;handle_=invalid_socket;return result;}
void SocketHandle::reset(const NativeSocket replacement) noexcept {
    if(valid()) {
#ifdef _WIN32
        closesocket(handle_);
#else
        close(handle_);
#endif
    }
    handle_=replacement;
}
bool SocketHandle::shutdown_both() noexcept {
    if(!valid()) return false;
#ifdef _WIN32
    return shutdown(handle_,SD_BOTH)==0;
#else
    return shutdown(handle_,SHUT_RDWR)==0;
#endif
}

}  // namespace silicon_switch::transport
