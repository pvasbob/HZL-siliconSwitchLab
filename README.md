# Silicon Switch Lab

Silicon Switch Lab is a cross-platform C++20 and Python learning project that
models the software stack surrounding a programmable network ASIC. It is being
built as practical preparation for a Cisco Silicon One software-engineering
interview, with an emphasis on modern C++, networking fundamentals,
hardware-aware design, testing, debugging, and measurable performance.

This is an educational software switch/router. It is not production networking
software and does not implement Cisco proprietary technology.

## Project goals

The completed system will model the path from network configuration to packet
forwarding:

```text
Python scenarios and configuration CLI
                |
Control plane and SAI-inspired API
                |
Hardware abstraction and state reconciliation
                |
C++ forwarding pipeline ("software ASIC")
                |
Virtual ports, queues, counters, and packet traces
```

The first versions run deterministically within one process. A later milestone
will add a versioned UDP transport so the simulated switch, control client, and
traffic endpoints can run across the available Linux and Windows computers
without requiring raw-socket privileges.

## Current implementation

The project currently provides:

- A standalone CMake-based C++20 build
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
- Tests for valid input, malformed input, boundary values, representation,
  formatting, classification, and comparisons

The current test executable runs 59 checks.

The next component is `Ipv4Prefix`, which will introduce subnet masks, prefix
normalization, address membership, and the foundation for longest-prefix route
matching.

## Planned capabilities

Later milestones will add:

- IPv4 prefixes and longest-prefix matching
- Explicit host/network-byte-order helpers
- Safe Ethernet, VLAN, ARP, and IPv4 parsing and serialization
- Virtual ports and IEEE 802.1Q VLAN membership
- Capacity-bounded MAC learning and aging
- Known-unicast forwarding and unknown-unicast/broadcast flooding
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
- An optional CUDA batch-processing performance experiment

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
- A C++20 compiler such as GCC or Clang
- A build tool supported by CMake

### Windows

- CMake 3.20 or newer
- Visual Studio 2022 with the Desktop development with C++ workload, or another
  C++20-capable compiler

Python will become a runtime requirement when the automation milestone begins.
CUDA will remain optional and will never be required for the core switch.

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

## Available three-computer lab

The distributed design targets the computers currently available:

- **Linux desktop**
  - Primary development and correctness environment
  - Runs the `SoftwareAsic` switch process
  - Runs deterministic simulations, sanitizers, profiling, stress tests, and
    high-load benchmarks
- **Windows laptop 1**
  - Runs the Python control-plane client and scenario tools
  - Applies topology, VLAN, route, and ACL configuration
  - Collects counters and desired/observed-state snapshots
- **Windows laptop 2**
  - Runs traffic endpoints and compatibility tests
  - Injects malformed traffic, endpoint failures, delays, and reordered updates

Roles will remain configurable. Multiple endpoint processes can run on one
machine when a scenario needs more logical hosts, and the deterministic
single-computer mode remains the primary correctness environment.

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
- C++20 concepts and constrained generic code
- Lambdas, algorithms, and ranges
- `std::optional`, structured result types, and explicit error handling
- `constexpr`, comparison operators, and compile-time validation
- Abstract base classes and runtime polymorphism at hardware/transport
  boundaries
- Threads, mutexes, condition variables, atomics, and explicit shutdown
- Cache-aware data layout and measurement-driven optimization

Features will not be inserted solely to make the code look complicated. Each
use must have a clear ownership, correctness, abstraction, or performance
reason.

## Testing strategy

The completed project will include:

- Unit tests for value types, parsing, tables, queues, and serialization
- Integration tests for same-VLAN switching and inter-VLAN routing
- Malformed-packet and resource-exhaustion tests
- Fault injection around hardware-programming operations
- Concurrency stress tests and Linux ThreadSanitizer runs
- Cross-platform tests on the Linux desktop and both Windows laptops
- Reproducible throughput, latency, lookup, and update benchmarks

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
   Cisco hardware experience.

## Safety and scope

The future distributed lab should run only on a trusted private network. Control
and data sockets will bind to explicitly configured interfaces and use
non-privileged ports. The educational control service should not be exposed to
the public Internet without adding appropriate authentication and transport
security.
