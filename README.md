# Silicon Switch Lab

A cross-platform C++20 and Python learning project that models the software
stack around a programmable network ASIC. It is designed as practical
preparation for a Cisco Silicon One software interview.

The project is intentionally built in layers:

```text
Python scenarios / CLI
        |
Control plane and SAI-like API
        |
Hardware abstraction and reconciliation
        |
C++ forwarding pipeline ("software ASIC")
        |
Virtual ports, queues, counters, and packet traces
```

The first version runs deterministically in one process. Later milestones add a
UDP transport so Linux and Windows machines can participate in one distributed
lab without requiring raw-socket privileges.

## What you will learn

- Modern C++: RAII, ownership, value types, STL, move semantics, error handling
- Networking: Ethernet, MAC learning, VLANs, ARP, IPv4, LPM, TTL, ECMP
- Systems: byte order, packed wire formats, queues, bounded resources, threads
- ASIC concepts: parse/match/action, tables, counters, finite capacity
- Reliability: desired versus observed state, idempotency, reconciliation
- Testing: unit, integration, stress, fault injection, and performance tests
- Python: scenario automation, configuration, traffic generation, log analysis
- SDLC: requirements, design records, implementation, CI, release, observability

See [PROJECT_PLAN.txt](PROJECT_PLAN.txt) for the complete roadmap and interview
question mapping.

## Planned machine roles

- Linux desktop: primary switch process and performance/load testing
- Linux laptop: traffic generator and packet/counter observer
- Windows laptop 1: control-plane client and Python scenario runner
- Windows laptop 2: second endpoint, failure injection, and compatibility test

Roles remain configurable, so the project works when only one computer is
available.

## Ground rules

1. Correctness comes before optimization.
2. Every feature starts with explicit behavior and failure semantics.
3. Hardware resources are finite and resource exhaustion is tested.
4. Software state is never assumed to equal hardware state; it is verified.
5. Performance claims require reproducible measurements.
6. Each defect should produce a retained regression test.

