# Interview demonstration

The demonstration has a deterministic headless part and a four-computer visual
part. Run the headless part first so forwarding correctness never depends on a
GPU, network, or window system.

## Scripted forwarding and recovery story

```bash
./build-cuda/silicon_switch_demo
```

The executable emits JSON evidence for six phases:

1. VLAN isolation: a VLAN 10 broadcast cannot reach the VLAN 20 port.
2. MAC learning: a learned destination becomes known unicast.
3. IPv4 routing: longest-prefix match selects port 3, decrements TTL, and
   rewrites Ethernet addresses.
4. Congestion: a capacity-one egress queue drops its second packet.
5. Failure: deterministic failed-port injection drops traffic.
6. Recovery: clearing the fault restores forwarding.

CTest also runs this scenario as `silicon_switch_interview_demo`, so the demo is
protected against regressions.

## Four-computer visual story

1. Show the lab manifest and identify the RTX leader and three observers.
2. Start all observer windows using the commands in `SCENARIO_34.md`.
3. Start the CUDA server for 900 ticks and point it at all four observer ports.
4. Explain snapshots, deltas, session IDs, sequences, and revisions while the
   topology updates.
5. Close each window after publishing stops and compare revision, resync, and
   FPS summaries.
6. Run the reliability CTest to demonstrate intentional loss, malformed input,
   restart, and recovery without disturbing the physical lab.
7. Show CPU/CUDA and cross-machine results from `BENCHMARKS.md`; explicitly
   explain why three objects are too small to amortize CUDA overhead.

## Suggested eight-minute recording

- 0:00–0:45: objective and four-machine architecture diagram
- 0:45–2:30: deterministic L2/L3/fault demo
- 2:30–4:30: four synchronized OpenGL windows
- 4:30–5:30: deliberate reliability/recovery test
- 5:30–6:30: CPU/CUDA and LAN benchmark evidence
- 6:30–7:30: C++ ownership, protocol, and threading choices
- 7:30–8:00: limitations and next production steps

Capture one screenshot containing the RTX terminal plus all three peer windows,
and one close-up of matching final revisions. Do not stage screenshots as test
evidence; retain the terminal summaries and CTest report alongside them.

## Interview talking points

- Why strong value types and variants prevent ambiguous error handling
- Why VLAN belongs in a MAC-table key and how LPM selects a route
- Why finite queues and tables make resource failure testable
- Why UDP plus revisioned snapshots fits replaceable visualization state
- How RAII owns sockets, CUDA memory, threads, and process logs
- What CUDA overhead revealed and how measurement prevented a false speedup
- Which production features are intentionally absent: authentication,
  encryption, durable reconciliation, and production-scale table algorithms
