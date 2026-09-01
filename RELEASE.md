# Release and final acceptance

Version 1.0 packages the completed educational four-computer synchronization
lab. The repository remains the source of truth; generated build directories,
packages, screenshots, and benchmark artifacts should not be committed.

## One-time final verification

After the final commit, pull it on the RTX desktop, Linux #2, Windows #1, and
Windows #2. Build and run CTest once on each machine using the commands in the
README or `CMakePresets.json`. The expected registered suites are:

- unit tests
- scripted interview demo
- Python orchestration tests
- four-process synchronization integration
- benchmark smoke test
- reliability/recovery integration

Then perform one physical four-computer run from `SCENARIO_34.md`. Acceptance
requires every observer to receive snapshots, reach the server's final
revision, and report its FPS. Run each RTX-to-peer UDP test from `BENCHMARKS.md`
once; retain loss and RTT JSON. This single session is the only validation that
cannot be completed on the development computer.

## Sanitizer verification on Linux

```bash
cmake --preset linux-sanitized
cmake --build --preset linux-sanitized --parallel
ctest --preset linux-sanitized
```

AddressSanitizer checks invalid memory access and leaks; UndefinedBehaviorSanitizer
checks undefined operations. CUDA is deliberately disabled in this preset.

## Install and package

```bash
cmake --preset linux-release
cmake --build --preset linux-release --parallel
ctest --preset linux-release
cmake --install build/linux-release --prefix staging
cpack --config build/linux-release/CPackConfig.cmake
```

CPack produces `.tar.gz` and `.zip` archives containing the C++17 library,
command-line applications, public headers, lab configurations, and documents.
On Windows, build the Release configuration and run CPack with `-C Release`.

## Release checklist

- Working tree contains only intended source changes.
- CPU Release and sanitizer builds pass all registered tests.
- CUDA target compiles and the RTX runtime reports backend `cuda`.
- Both Windows laptops compile with MSVC and pass all tests.
- Linux #2 passes all tests.
- Four physical observers reach the same expected revision.
- Benchmark JSON, FPS summaries, and screenshots are archived outside Git.
- Package contents and version output are inspected.
- Known limitations in `ARCHITECTURE.md` are presented honestly.

## Resume-ready description

Built a cross-platform C++17 software-switch and distributed visualization lab
that synchronizes versioned topology state from an RTX/CUDA Linux server to
Linux and Windows OpenGL observers; implemented VLAN/MAC learning, IPv4 LPM
routing, bounded queues, counters, fault injection, loss/restart recovery,
portable socket RAII, automated tests, sanitizers, CI, and reproducible CPU/GPU
and LAN benchmarks.

Short form:

> Built and tested a C++17/CUDA distributed switch simulator across Linux and
> Windows with revision-safe UDP synchronization and fault recovery.
