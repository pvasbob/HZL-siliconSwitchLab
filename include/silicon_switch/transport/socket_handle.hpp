#pragma once

#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

namespace silicon_switch::transport {

#ifdef _WIN32
using NativeSocket = SOCKET;
inline constexpr NativeSocket invalid_socket = INVALID_SOCKET;
#else
using NativeSocket = int;
inline constexpr NativeSocket invalid_socket = -1;
#endif

struct SocketError {
    int code{0};
    std::string message;
};

[[nodiscard]] SocketError last_socket_error();

class SocketRuntime {
public:
    SocketRuntime();
    ~SocketRuntime();
    SocketRuntime(const SocketRuntime&) = delete;
    SocketRuntime& operator=(const SocketRuntime&) = delete;

private:
    bool initialized_{false};
};

// Ensures Winsock remains initialized for the process lifetime. On POSIX this
// is a harmless no-op runtime object.
void ensure_socket_runtime();

class SocketHandle {
public:
    SocketHandle() noexcept = default;
    explicit SocketHandle(NativeSocket handle) noexcept : handle_{handle} {}
    ~SocketHandle();

    SocketHandle(SocketHandle&& other) noexcept;
    SocketHandle& operator=(SocketHandle&& other) noexcept;
    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return handle_ != invalid_socket;
    }
    [[nodiscard]] constexpr NativeSocket native_handle() const noexcept {
        return handle_;
    }
    [[nodiscard]] NativeSocket release() noexcept;
    void reset(NativeSocket replacement = invalid_socket) noexcept;
    [[nodiscard]] bool shutdown_both() noexcept;

private:
    NativeSocket handle_{invalid_socket};
};

}  // namespace silicon_switch::transport
