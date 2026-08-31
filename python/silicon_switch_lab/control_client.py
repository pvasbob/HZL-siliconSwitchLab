"""Client for the versioned, length-framed switch control protocol."""

from __future__ import annotations

import enum
import socket
import struct
from dataclasses import dataclass


class ControlMessageType(enum.IntEnum):
    CREATE_PORT = 1
    REMOVE_PORT = 2
    SET_PORT_STATE = 3
    CREATE_VLAN = 10
    REMOVE_VLAN = 11
    ADD_VLAN_MEMBER = 12
    SET_ROUTER_INTERFACE = 20
    ADD_OR_REPLACE_ROUTE = 21
    REMOVE_ROUTE = 22
    ADD_OR_REPLACE_NEIGHBOR = 23
    REMOVE_NEIGHBOR = 24
    CONFIGURE_FAULTS = 30
    QUERY_COUNTERS = 40
    RESPONSE = 100


@dataclass(frozen=True)
class ControlMessage:
    message_type: ControlMessageType
    request_id: int
    payload: bytes = b""

    MAGIC = 0x53534350
    VERSION = 1
    HEADER = struct.Struct("!IHHII")
    MAXIMUM_PAYLOAD = 1_048_576

    def encode(self) -> bytes:
        if not 0 <= self.request_id <= 0xFFFFFFFF:
            raise ValueError("request_id is outside the uint32 range")
        if len(self.payload) > self.MAXIMUM_PAYLOAD:
            raise ValueError("control payload is too large")
        return self.HEADER.pack(
            self.MAGIC,
            self.VERSION,
            int(self.message_type),
            self.request_id,
            len(self.payload),
        ) + self.payload

    @classmethod
    def decode(cls, data: bytes) -> "ControlMessage":
        if len(data) < cls.HEADER.size:
            raise ValueError("truncated control message")
        magic, version, raw_type, request_id, length = cls.HEADER.unpack_from(data)
        if magic != cls.MAGIC:
            raise ValueError("invalid control message magic")
        if version != cls.VERSION:
            raise ValueError("unsupported control protocol version")
        if length > cls.MAXIMUM_PAYLOAD or len(data) != cls.HEADER.size + length:
            raise ValueError("invalid control payload length")
        try:
            message_type = ControlMessageType(raw_type)
        except ValueError as error:
            raise ValueError("unknown control message type") from error
        return cls(message_type, request_id, data[cls.HEADER.size :])


class ControlClient:
    """Synchronous TCP client with request identifiers and exact framing."""

    FRAME = struct.Struct("!I")

    def __init__(self, host: str, port: int, timeout: float = 5.0) -> None:
        self._host = host
        self._port = port
        self._timeout = timeout
        self._socket: socket.socket | None = None
        self._next_request_id = 1

    def __enter__(self) -> "ControlClient":
        self.connect()
        return self

    def __exit__(self, *_: object) -> None:
        self.close()

    def connect(self) -> None:
        if self._socket is None:
            self._socket = socket.create_connection(
                (self._host, self._port), timeout=self._timeout
            )

    def close(self) -> None:
        if self._socket is not None:
            self._socket.close()
            self._socket = None

    def request(self, message_type: ControlMessageType, payload: bytes = b"") -> ControlMessage:
        if self._socket is None:
            raise RuntimeError("control client is not connected")
        request_id = self._next_request_id
        self._next_request_id = 1 if request_id == 0xFFFFFFFF else request_id + 1
        encoded = ControlMessage(message_type, request_id, payload).encode()
        self._socket.sendall(self.FRAME.pack(len(encoded)) + encoded)
        frame_length = self.FRAME.unpack(self._receive_exact(self.FRAME.size))[0]
        if frame_length > ControlMessage.HEADER.size + ControlMessage.MAXIMUM_PAYLOAD:
            raise ValueError("control response frame is too large")
        response = ControlMessage.decode(self._receive_exact(frame_length))
        if response.message_type != ControlMessageType.RESPONSE:
            raise ValueError("server returned a non-response message")
        if response.request_id != request_id:
            raise ValueError("control response request_id does not match")
        return response

    def _receive_exact(self, size: int) -> bytes:
        assert self._socket is not None
        chunks = bytearray()
        while len(chunks) < size:
            chunk = self._socket.recv(size - len(chunks))
            if not chunk:
                raise ConnectionError("control connection closed")
            chunks.extend(chunk)
        return bytes(chunks)
