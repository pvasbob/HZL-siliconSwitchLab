# Silicon Switch Lab

> **Project status: IN PROGRESS**
>
> The repository is being developed incrementally. The
> [Current implementation](#current-implementation) section identifies what
> works today; the [Final product goal](#final-product-goal) and
> [Definition of final success](#definition-of-final-success) describe the
> intended completed system.

Silicon Switch Lab is a cross-platform C++17, Python, CUDA, OpenGL, and
networking project that models the software stack surrounding a programmable
network ASIC and uses that stack to support a distributed GPU visualization
application. It is being built as a general systems-engineering portfolio and
learning project, with an emphasis on modern C++, L2/L3 networking,
hardware-aware design, distributed systems, testing, debugging, and measurable
performance.

This is an educational software switch/router. It is not production networking
software and does not implement any vendor's proprietary technology.

## What the finished project will accomplish

The completed project will let one Linux workstation run an authoritative
CPU/CUDA simulation while four independently built Linux and Windows
applications display synchronized OpenGL views of the same evolving scene. The
GPU workstation will control the simulation; the other computers will be
read-only observers with independent or leader-following cameras.

Scene updates will not bypass the networking work. They will be serialized into
a versioned protocol and carried through a hardware-inspired `SoftwareAsic`
that implements Ethernet and VLAN parsing, MAC learning, L2 switching, ARP,
IPv4 routing, longest-prefix matching, next hops, ACLs, bounded queues,
congestion drops, and counters. Loss, reordering, disconnection, hardware
failure, and state divergence will be detectable and testable.

A SAI-inspired control layer and `HardwareInterface` class hierarchy will
separate portable control-plane intent from the software-ASIC and
fault-injecting hardware implementations. Python tools will configure the
four-computer lab, run repeatable scenarios, collect telemetry, and generate
machine-readable reports. A deterministic CPU backend will serve as the
correctness reference for optional CUDA acceleration, and headless tests will
keep networking correctness independent of the GPU and OpenGL renderer.

The final demonstration will show this complete path:

```text
Leader changes simulation state on Linux GPU workstation
        |
CPU/CUDA backend produces authoritative scene revision
        |
Versioned scene update is serialized
        |
SoftwareAsic switches or routes it through tested L2/L3 behavior
        |
Linux and Windows observers validate and apply the revision
        |
Four OpenGL windows render the synchronized scene
```

The engineering evidence will include cross-platform builds, unit and
integration tests, fault injection, sanitizer runs, profiling, and reproducible
CPU, GPU, rendering, serialization, and networking measurements.

## Final product goal

The final product is a distributed GPU simulation studio for a four-computer
engineering team:

- One Linux desktop with a CUDA-capable GPU runs and modifies the authoritative
  simulation.
- The Linux desktop also presents the leader's OpenGL view.
- One Linux laptop and two Windows laptops run read-only OpenGL observer
  applications.
- Every observer receives synchronized scene state and can render the same
  simulation from an independently controlled camera.
- The observer computers do not need CUDA or a high-performance discrete GPU;
  they render a capability-appropriate representation of results computed on
  the Linux GPU server.

```text
                         Linux GPU desktop
               +----------------------------------+
               | Leader controls and OpenGL view |
               | Authoritative simulation server |
               | CPU reference / CUDA backend    |
               | Scene snapshot publisher        |
               +----------------+-----------------+
                                |
                    Versioned scene protocol
                                |
                       SoftwareAsic network
                                |
             +------------------+------------------+
             |                  |                  |
             v                  v                  v
      Linux laptop       Windows laptop 1   Windows laptop 2
      OpenGL observer    OpenGL observer    OpenGL observer
```

This gives the network stack a concrete workload. GPU-generated scene snapshots
must be serialized, switched or routed, queued, measured, delivered, and
resynchronized across heterogeneous computers.

The project answers a practical engineering question:

> How can one GPU workstation run an expensive simulation while a distributed
> team safely observes synchronized results on ordinary Linux and Windows
> computers, with explicit behavior under bandwidth limits, congestion, packet
> loss, endpoint failure, and state divergence?

## System architecture

The completed system will model the complete path from network configuration
and GPU computation to distributed visualization:

```text
Python deployment, configuration, scenarios, and reports
                            |
             Desired-state control plane
                            |
                  SAI-inspired API
                            |
                  HardwareInterface
                    /             \
                   v               v
             SoftwareAsic   FaultInjectingHardware
                            |
           L2/L3 forwarding, queues, and counters
                            |
              Versioned scene-state protocol
                            |
       Authoritative CPU/CUDA simulation server
                            |
             OpenGL leader and observers
```

The simulation core, switch core, networking protocol, and renderer remain
separate components. Correctness tests can run headlessly without CUDA, OpenGL,
or a physical multi-computer lab.

The first networking versions run deterministically within one process. A later
milestone will add a versioned UDP transport so the simulated switch, control
client, simulation server, and observers can run across the available Linux and
Windows computers without requiring raw-socket privileges.

Python will integrate with the C++ system through two explicit boundaries:

- Subprocess automation will build, launch, monitor, and stop C++ services and
  observer applications while collecting logs and test artifacts.
- A versioned socket control protocol will configure the long-running C++
  `SoftwareAsic`, query state and counters, and run scenarios locally or across
  the four-computer lab.

The forwarding path and authoritative switch state remain in C++. Python does
not become an in-process dependency of packet processing, and direct Python/C++
bindings or an embedded interpreter are not required by the current design.

## Current implementation

The project currently provides:

- A standalone CMake-based C++17 build
- A reusable `silicon_switch` static library
- A small CLI linked against the library
- A dependency-free unit-test executable registered with CTest
- Strict GCC, Clang, and MSVC warning settings
- A strongly typed `MacAddress` value type
  - validated uppercase and lowercase hexadecimal parsing
  - canonical uppercase formatting
  - byte access, equality, and ordering
  - unicast, multicast, and broadcast classification
- A strongly typed `Ipv4Address` value type
  - validated dotted-decimal parsing
  - canonical formatting
  - four-octet and packed 32-bit representations
  - equality and numeric ordering
  - unspecified, loopback, multicast, and limited-broadcast classification
- A validated `Ipv4Prefix` value type
  - CIDR parsing and canonical formatting
  - network-address normalization
  - subnet-mask and last-address calculation
  - address membership checks
  - `/0` default-route and `/32` host-route support
- Header-only C++17 big-endian byte-order utilities
  - constrained unsigned-integer templates
  - checked reads and all-or-nothing writes
  - 8-, 16-, 32-, and 64-bit wire values
  - host-endianness-independent behavior
- A strongly typed `EtherType` representation
  - named IPv4, ARP, 802.1Q VLAN, and IPv6 values
  - preservation of unknown protocol identifiers
- An owning, validated Ethernet/IPv4 `ArpPacket` value type
  - separate request and reply construction
  - sender and target MAC and IPv4 address fields
  - exact 28-byte parsing and network-byte-order serialization
  - support for ARP probes with an unspecified sender IPv4 address
  - rejection of unsupported hardware/protocol types, invalid address lengths,
    invalid operations, malformed MAC roles, truncation, and trailing bytes
- A validated IPv4-to-MAC `ArpCache`
  - explicit inserted, replaced, and rejected update outcomes
  - neighbor lookup and removal
  - deterministic size and empty-state queries
  - rejection of unspecified, multicast, and broadcast addresses
  - configurable entry lifetime based on `std::chrono::steady_clock`
  - timestamp refresh when mappings are replaced
  - time-aware lookup and explicit aging passes
  - deterministic expiration-boundary tests without sleeps or wall-clock time
- An owning, validated `EthernetFrame` value type
  - destination and source MAC addresses
  - EtherType and variable-size payload
  - checked parsing from non-owning byte spans
  - serialization into network byte order
  - rejection of truncated headers and oversized payloads
- Validated IEEE 802.1Q VLAN value types
  - configurable VLAN identifiers from 1 through 4094
  - three-bit Priority Code Point and Drop Eligible Indicator
  - 16-bit Tag Control Information encoding and decoding
  - explicit rejection of reserved or unsupported VLAN identifiers
- VLAN-aware Ethernet frames
  - support for both 14-byte untagged and 18-byte tagged headers
  - automatic `0x8100` Tag Protocol Identifier serialization
  - inner EtherType preservation
  - checked tagged-frame parsing and serialization
  - rejection of truncated and malformed VLAN tags
- Strongly typed IPv4 protocol identifiers for ICMP, TCP, and UDP
- Reusable Internet-checksum utilities
  - checksum generation for IPv4 headers before transmission
  - validation of received headers containing a stored checksum
  - correct one's-complement carry folding
  - support for both even- and odd-length byte sequences
- An owning, validated `Ipv4Packet` value type
  - creation, checked parsing, and network-byte-order serialization
  - fixed 20-byte IPv4 headers with source, destination, protocol, TTL,
    identification, and Don't Fragment fields
  - automatic total-length and header-checksum generation
  - checksum, length, version, TTL, and fragmentation validation
  - explicit rejection of unsupported IPv4 options and fragmented packets
- Strongly typed IPv4 routes and longest-prefix matching
  - validated output-port and next-hop values
  - exact lookup, insertion, replacement, and removal
  - default, network, subnet, and host routes
  - direct routes and gateway routes
- Explicit IPv4 forwarding result types
  - `ForwardedIpv4Packet` owns the resulting packet and identifies its output
    port and resolved next-hop address
  - `DroppedIpv4Packet` reports a typed TTL-expired or route-miss reason
  - `Ipv4ForwardingResult` uses `std::variant` so callers must handle either
    forwarding or dropping without null pointers or ambiguous sentinel values
- Controlled IPv4 TTL updates
  - packets with TTL greater than one produce a new packet with TTL reduced by
    exactly one
  - packets with TTL equal to one produce a typed TTL-expired drop
  - source, destination, protocol, payload, identification, and fragmentation
    policy are preserved while the input packet remains unchanged
  - serialization regenerates and validates the IPv4 header checksum
- An owning `Ipv4ForwardingEngine`
  - applies TTL expiration and immutable TTL decrement
  - performs longest-prefix route selection
  - selects the configured gateway for indirect routes and the packet
    destination for directly connected routes
  - returns explicit forwarded or TTL-expired/route-miss outcomes
- Layer 3 IPv4-to-Ethernet encapsulation
  - resolves direct or gateway next hops through the aging-aware ARP cache
  - uses the router egress MAC as source and resolved neighbor MAC as
    destination
  - constructs a fresh untagged IPv4 Ethernet frame without mutating its input
  - reports invalid source MAC, unresolved neighbor, and oversized packet
    failures explicitly
- A capacity-bounded, VLAN-aware `MacTable`
  - keys entries by VLAN and MAC address
  - distinguishes dynamic and static entries
  - reports insertion, refresh, movement, capacity exhaustion, invalid source,
    and static-conflict outcomes
  - preserves independent mappings for identical MAC addresses across VLANs
  - expires dynamic entries after a configurable monotonic-clock lifetime while
    retaining static entries
  - refresh and movement update deterministic learning timestamps
- Validated VLAN port configuration
  - models access ports with one access VLAN
  - models trunks with allowed VLAN sets and optional native VLANs
  - rejects empty trunk VLAN sets and native VLANs outside the allowed set
- VLAN-aware ingress classification
  - assigns untagged access traffic to the access VLAN
  - accepts allowed tagged trunk traffic and maps untagged trunk traffic to the
    configured native VLAN
  - reports tagged-access, missing-native-VLAN, and disallowed-VLAN drops
  - preserves the input Ethernet frame
- Layer 2 forwarding decisions
  - forwards known unicast traffic to its learned VLAN-local port
  - floods unknown unicast, broadcast, and multicast traffic only to eligible
    VLAN ports and never back to ingress
  - filters same-port destinations, out-of-VLAN learned ports, and floods with
    no eligible egress
- Integrated Layer 2/Layer 3 ingress classification
  - combines VLAN validation, source learning, router-interface ownership, and
    Layer 2 destination lookup
  - selects known-unicast switching, flooding, IPv4 routing, local ARP control,
    or an explicit drop reason
  - protects static MAC bindings from source movement
- A validated `VirtualPort` model
  - represents identity, MAC address, administrative and link state, speed,
    MTU, and VLAN configuration
  - records receive/transmit packets, bytes, errors, and drops
  - rejects invalid MAC addresses and MTU values
- Tests for valid input, malformed input, boundary values, representation,
  formatting, classification, byte order, ownership, VLAN bit fields,
  tagged/untagged serialization, Internet checksums, and IPv4 packet
  round trips

The current test executable runs 462 checks.

The next milestone will add bounded packet queues with explicit congestion-drop
behavior and close/shutdown semantics.

## Planned capabilities

Later milestones will add:

- ARP/neighbor state and IPv4 routing
- Route, next-hop, and ECMP objects
- ACL-style match/action rules
- Bounded ingress and egress queues
- Per-stage counters and explicit congestion-drop reasons
- Thread-safe worker pipelines and graceful shutdown
- A SAI-inspired hardware interface
- A `SoftwareAsic` implementation and fault-injecting hardware model
- Desired-state and observed-state reconciliation
- Python topology, configuration, traffic, and report tools
- A distributed UDP virtual-port transport
- Unit, integration, concurrency, fault-injection, and benchmark suites
- A deterministic headless simulation core
- A CPU reference simulation backend
- An optional CUDA simulation backend on the Linux GPU desktop
- CPU/GPU correctness comparison and performance measurement
- Versioned full-scene snapshots and incremental scene updates
- Scene revision, sequence, timestamp, and resynchronization handling
- Linux and Windows OpenGL observer applications
- Independent and leader-following camera modes
- Capability negotiation and level-of-detail selection for observer computers
- Bandwidth-aware sampling, quantization, or compression experiments
- An optional encoded-video fallback for scenes too large to replicate

See [PROJECT_PLAN.txt](PROJECT_PLAN.txt) for the detailed roadmap, exit criteria,
test strategy, and interview-question mapping.

## Repository layout

```text
siliconSwitchLab/
├── apps/
│   └── switch_cli/              CLI executable entry point
├── include/
│   └── silicon_switch/          Public library interfaces
│       └── network/             Networking value types
├── src/
│   └── silicon_switch/          Library implementations
│       └── network/
├── tests/
│   └── network/                 Component unit tests
├── docs/                        Learning, architecture, and design notes
├── CMakeLists.txt
├── PROJECT_PLAN.txt
└── README.md
```

Public library headers and private implementations are kept separate. As the
project grows, components will be split by responsibility rather than collected
into oversized source files.

## Requirements

### Linux

- CMake 3.20 or newer
- A C++17 compiler such as GCC or Clang
- A build tool supported by CMake

### Windows

- CMake 3.20 or newer
- Visual Studio 2022 with the Desktop development with C++ workload, or another
  C++17-capable compiler

Python will become a runtime requirement when the automation milestone begins.
CUDA will remain optional and will never be required for the core switch, CPU
reference simulation, or observer applications. Later visualization milestones
will introduce an OpenGL-capable windowing library and OpenGL function loader as
explicit, documented dependencies.

## Build and test on Linux

From the repository root:

```bash
cmake -S . -B build
cmake --build build
```

Run the CLI:

```bash
./build/silicon_switch_cli
```

Expected output:

```text
Silicon Switch Lab 0.1.0
```

Run the tests directly:

```bash
./build/silicon_switch_tests
```

Run the registered CTest suite:

```bash
ctest --test-dir build --output-on-failure
```

## Build and test on Windows

From a Developer PowerShell or Developer Command Prompt in the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

With the default multi-configuration Visual Studio generator, run:

```powershell
.\build\Debug\silicon_switch_cli.exe
.\build\Debug\silicon_switch_tests.exe
```

Single-configuration generators such as Ninja may place the executables directly
under `build` instead.

## Available four-computer lab

The distributed design targets the computers currently available:

- **Linux desktop**
  - Primary development and correctness environment
  - Runs the `SoftwareAsic` switch process
  - Runs the authoritative simulation server and CUDA backend
  - Runs the leader's OpenGL controls and visualization
  - Publishes versioned scene snapshots to observers
  - Runs deterministic simulations, sanitizers, profiling, stress tests, and
    high-load benchmarks
- **Linux laptop**
  - Runs a read-only OpenGL observer with an independent camera
  - Runs the first traffic generator and packet/counter observer
  - Validates scene ordering, loss, interpolation, and resynchronization
  - Uses tools such as tcpdump or Wireshark to inspect the UDP lab transport
- **Windows laptop 1**
  - Runs a read-only OpenGL observer
  - Runs the Python control-plane client and scenario tools when required
  - Applies topology, VLAN, route, and ACL configuration
  - Collects counters and desired/observed-state snapshots
- **Windows laptop 2**
  - Runs a read-only OpenGL observer
  - Runs traffic endpoints, compatibility tests, and fault scenarios
  - Injects malformed traffic, endpoint failures, scene-update delays, and
    reordered updates

Roles will remain configurable. Multiple endpoint processes can run on one
machine when a scenario needs more logical hosts, and the deterministic
single-computer mode remains the primary correctness environment.

## Distributed scene model

OpenGL contexts are local to each process and cannot be shared directly across
the network. The Linux server therefore publishes logical scene state rather
than attempting to share its OpenGL context.

```text
Static data loaded by every client:
    shaders, meshes, textures, topology, and scene metadata

Dynamic data published by the server:
    object transforms, particle samples, colors, simulation time,
    scene revision, and visualization mode
```

The Linux server is authoritative. Observer applications are read-only with
respect to simulation state, though they may control local cameras. A
presentation mode will allow observers to follow the leader's camera.

Every scene update will carry a protocol version, session identity, sequence
number, simulation tick, timestamp, revision, payload length, and integrity
check. A client that detects a missing revision stops applying dependent deltas
and requests a complete snapshot before resuming.

For very large GPU simulations, the server will not transmit every particle at
every rendered frame. The project will measure and compare approaches such as:

- Reduced snapshot frequency with client interpolation
- Particle sampling and level of detail
- Quantized positions and attributes
- Full snapshots versus incremental deltas
- Compression
- Per-client quality selection
- Encoded video as an optional fallback

The goal is to make CPU/GPU/network tradeoffs observable rather than assuming
that one transport strategy is universally best.

## Networking model

The forwarding pipeline will be built incrementally:

```text
Packet enters a virtual port
        |
Validate available bytes
        |
Parse Ethernet and optional VLAN headers
        |
Determine ingress VLAN
        |
Learn source MAC address
        |
Apply ingress policy
        |
Bridge lookup or IPv4 route lookup
        |
Select output port or next hop
        |
Rewrite required headers
        |
Enqueue in a bounded egress queue
        |
Schedule, transmit, and update counters
```

Every parser must validate lengths before reading. Every table and queue will
have explicit capacity. Drops and programming failures will report structured
reasons rather than silently losing state.

## Modern C++ and interview focus

The project deliberately exercises interview-relevant C++ features when they
fit the design:

- Strong value types and encapsulation
- RAII and deterministic cleanup
- Value semantics, copy elision, and move semantics
- `std::unique_ptr` for exclusive ownership
- `std::shared_ptr` and `std::weak_ptr` only where shared lifetime is justified
- Function and class templates
- C++17 templates with compile-time constraints
- Lambdas, algorithms, and ranges
- `std::optional`, structured result types, and explicit error handling
- `constexpr`, comparison operators, and compile-time validation
- Abstract base classes and runtime polymorphism at hardware/transport
  boundaries
- Move-only RAII wrappers for OpenGL and CUDA resources
- `std::variant` messages and visitors for the scene/network protocol
- Threads, mutexes, condition variables, atomics, and explicit shutdown
- Cache-aware data layout and measurement-driven optimization

Features will not be inserted solely to make the code look complicated. Each
use must have a clear ownership, correctness, abstraction, or performance
reason.

## Testing strategy

The completed project will include:

- Unit tests for value types, parsing, tables, queues, and serialization
- Integration tests for same-VLAN switching and inter-VLAN routing
- Deterministic CPU simulation and CPU/GPU comparison tests
- Scene-protocol serialization, revision-gap, and resynchronization tests
- Cross-platform OpenGL capability and rendering smoke tests
- Malformed-packet and resource-exhaustion tests
- Fault injection around hardware-programming operations
- Concurrency stress tests and Linux ThreadSanitizer runs
- Distributed and cross-platform tests on both Linux computers and both Windows
  laptops
- Reproducible forwarding, simulation, serialization, snapshot-bandwidth,
  latency, lookup, and update benchmarks

Every discovered defect should result in a retained regression test.

## Ground rules

1. Correctness comes before optimization.
2. Every feature begins with explicit behavior, invariants, and failure
   semantics.
3. Hardware resources are finite, and resource exhaustion is tested.
4. Software state is not assumed to equal hardware state; it is observed and
   reconciled.
5. The control plane must not hold a state lock across a slow hardware call.
6. Performance claims require reproducible measurements.
7. Optimizations must remain protected by correctness tests.
8. The project documents limitations honestly and does not claim production
   network-hardware experience.
9. OpenGL remains an observer of authoritative state and never becomes a
   dependency of forwarding correctness.
10. CUDA acceleration remains optional and is always checked against a CPU
    reference implementation.

## Safety and scope

The future distributed lab should run only on a trusted private network. Control
and data sockets will bind to explicitly configured interfaces and use
non-privileged ports. The educational control service should not be exposed to
the public Internet without adding appropriate authentication and transport
security.

## Definition of final success

The final project is successful when:

- One Linux GPU workstation can run and modify an authoritative simulation.
- Three Linux/Windows observer applications display synchronized OpenGL views.
- Observers can use independent cameras or follow the leader's camera.
- The CPU and CUDA simulation backends agree within defined correctness
  tolerances.
- Scene transport detects loss, reordering, stale sessions, and revision gaps.
- Disconnected observers can reconnect and obtain a consistent full snapshot.
- Scene traffic passes through the software ASIC's L2/L3 forwarding pipeline.
- VLANs, IPv4 routes, bounded queues, counters, congestion, and failures affect
  traffic in explicitly tested ways.
- The SAI-inspired control plane can inject, detect, and reconcile
  desired/observed-state divergence.
- Python tools can deploy the four-computer lab, run scenarios, collect logs,
  and produce machine-readable reports.
- Sanitizer, fault-injection, and cross-platform test suites are repeatable.
- CPU, GPU, rendering, serialization, and networking performance claims are
  supported by reproducible measurements.
