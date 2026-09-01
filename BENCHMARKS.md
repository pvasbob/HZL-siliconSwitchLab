# Step 35: Four-computer performance benchmark

This milestone measures the same C++17 implementation on the RTX 3080 leader,
Linux laptop, and both Windows laptops. Commands emit one JSON object per run so
results can be archived or plotted without parsing human-readable logs. The lab
addresses and dedicated benchmark ports are recorded in
`config/benchmark_lab.json`.

## 1. Build and test every computer

On the RTX desktop:

```bash
cmake -S . -B build-cuda -DCMAKE_BUILD_TYPE=Release -DSILICON_SWITCH_ENABLE_CUDA=ON
cmake --build build-cuda --parallel
ctest --test-dir build-cuda --output-on-failure
```

On Linux #2:

```bash
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release -DSILICON_SWITCH_ENABLE_CUDA=OFF
cmake --build build-linux --parallel
ctest --test-dir build-linux --output-on-failure
```

On each Windows laptop, from a VS 2019 Developer Command Prompt:

```bat
cmake -S . -B build-win-gui -G "Visual Studio 16 2019" -A x64 -DSILICON_SWITCH_ENABLE_CUDA=OFF
cmake --build build-win-gui --config Release --parallel
ctest --test-dir build-win-gui -C Release --output-on-failure
```

## 2. Record local compute results

Run each command three times. On Linux, replace `BUILD` with `build-cuda` or
`build-linux`:

```bash
./BUILD/silicon_switch_benchmark simulation --backend cpu --iterations 100000 --observers 3
./BUILD/silicon_switch_benchmark forwarding --iterations 1000000 --payload 512
./BUILD/silicon_switch_benchmark render --iterations 100000 --nodes 100
./BUILD/silicon_switch_benchmark queue --iterations 5000000 --capacity 1024
```

Use the corresponding Windows executable:

```bat
build-win-gui\Release\silicon_switch_benchmark.exe simulation --backend cpu --iterations 100000 --observers 3
build-win-gui\Release\silicon_switch_benchmark.exe forwarding --iterations 1000000 --payload 512
build-win-gui\Release\silicon_switch_benchmark.exe render --iterations 100000 --nodes 100
build-win-gui\Release\silicon_switch_benchmark.exe queue --iterations 5000000 --capacity 1024
```

Additionally, on the RTX desktop only, compare CUDA against the CPU result:

```bash
./build-cuda/silicon_switch_benchmark simulation --backend cuda --iterations 100000 --observers 3
```

## 3. Permit the benchmark UDP ports

Run PowerShell as Administrator on Windows #1:

```powershell
New-NetFirewallRule -DisplayName "Silicon Switch Benchmark 8101" -Direction Inbound -Protocol UDP -LocalPort 8101 -Action Allow -Profile Private
```

Run the equivalent command on Windows #2, changing both occurrences of `8101`
to `8102`. Linux #2 currently has UFW inactive; if it is later enabled, allow
`8103/udp`.

## 4. Measure RTX-to-peer latency, loss, and effective bandwidth

Start one server at a time and leave it running. On Windows #1:

```bat
build-win-gui\Release\silicon_switch_benchmark.exe udp-server --listen 0.0.0.0 --port 8101 --packets 1000 --timeout-ms 30000
```

On the RTX desktop, run:

```bash
./build-cuda/silicon_switch_benchmark udp-client --target 192.168.12.199 --port 8101 --packets 1000 --payload 1024 --timeout-ms 1000
```

Repeat with Windows #2 using address `192.168.12.216` and port `8102`, then
Linux #2 using address `192.168.12.159` and port `8103`. The Linux #2 server is:

```bash
./build-linux/silicon_switch_benchmark udp-server --listen 0.0.0.0 --port 8103 --packets 1000 --timeout-ms 30000
```

Repeat every peer test with payload sizes `64`, `1024`, and `1400`, and run
each combination three times. This sequential echo test measures round-trip
latency, loss, and effective request/response bandwidth; it intentionally does
not claim peak one-way UDP throughput.

## 5. Measure real OpenGL FPS and synchronization quality

Run the step 34 observer/server scenario on all four computers for 900 ticks.
After the server finishes, close each observer window. Its final line now
includes `frames`, `fps`, snapshots, deltas, resyncs, and final revision. Passing
criteria are revision 900 on every observer, zero resyncs in a clean LAN run,
and an FPS result recorded for every display. A nonzero resync count is not
hidden: retain it as the packet-loss/recovery result and repeat the run.

## Acceptance criteria

- All four CTest suites pass on every machine.
- CPU results exist for all four machines and a CUDA result exists for the RTX
  desktop.
- All three RTX-to-peer paths report 1,000 received packets and zero loss in at
  least one clean run.
- Every OpenGL observer reaches revision 900 and reports measured FPS.
- Raw JSON output and observer summaries are retained with machine name, date,
  payload size, and run number.
