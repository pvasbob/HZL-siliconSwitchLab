# Four-computer integration scenario

This scenario connects the authoritative CUDA simulation on the Linux GPU
desktop to three independently built OpenGL observers.

## Lab endpoints

| Machine | Address | Node | State port |
|---|---:|---:|---:|
| Linux RTX 3080 leader | `192.168.12.185` | `1001` | `8000` |
| Windows observer 1 | `192.168.12.199` | `2001` | `8001` |
| Windows observer 2 | `192.168.12.216` | `2002` | `8002` |
| Linux observer | `192.168.12.159` | `3001` | `8003` |

All machines must pull the same commit and rebuild before the scenario.

## Firewall preparation

Run in an elevated PowerShell on Windows observer 1:

```powershell
New-NetFirewallRule -DisplayName "Silicon Switch State 8001" -Direction Inbound -Protocol UDP -LocalPort 8001 -Action Allow -Profile Private
```

Run in an elevated PowerShell on Windows observer 2:

```powershell
New-NetFirewallRule -DisplayName "Silicon Switch State 8002" -Direction Inbound -Protocol UDP -LocalPort 8002 -Action Allow -Profile Private
```

## Live OpenGL scenario

Start all four display processes before starting the server.

Linux RTX 3080 leader display:

```bash
./build-cuda/silicon_switch_observer --listen 0.0.0.0 --port 8000 --name gpu-desktop
```

Windows observer 1:

```cmd
build-win\Release\silicon_switch_observer.exe --listen 0.0.0.0 --port 8001 --name windows-laptop-1
```

Windows observer 2:

```cmd
build-win\Release\silicon_switch_observer.exe --listen 0.0.0.0 --port 8002 --name windows-laptop-2
```

Linux observer:

```bash
./build-linux/silicon_switch_observer --listen 0.0.0.0 --port 8003 --name linux-laptop
```

Linux RTX 3080 leader:

```bash
./build-cuda/silicon_switch_server \
  --backend cuda --hz 30 --ticks 900 \
  --observer 192.168.12.185:8000 \
  --observer 192.168.12.199:8001 \
  --observer 192.168.12.216:8002 \
  --observer 192.168.12.159:8003
```

All four windows must animate the same topology. After the server reports 900
published revisions, press Escape in every display. Each observer prints its
snapshot, delta, resynchronization, and final-revision counters.

## Verified physical result

The four-computer scenario was verified on August 31, 2026 with a 300-revision
CUDA run:

| Display | Snapshots | Deltas | Resync events | Final revision |
|---|---:|---:|---:|---:|
| RTX 3080 leader | 6 | 294 | 0 | 300 |
| Windows observer 1 | 6 | 294 | 0 | 300 |
| Windows observer 2 | 6 | 258 | 35 | 300 |
| Linux observer | 6 | 294 | 0 | 300 |

Windows observer 2 detected UDP loss, rejected unsafe deltas, and recovered at
a periodic snapshot. All four displays converged on authoritative revision 300.

## Headless acceptance run

For machine-readable evidence, repeat the scenario using
`silicon_switch_observer_node` instead of the OpenGL application.

Windows observer 1:

```cmd
build-win\Release\silicon_switch_observer_node.exe --listen 0.0.0.0 --port 8001 --name windows-laptop-1 --updates 300 --timeout-ms 30000
```

Windows observer 2:

```cmd
build-win\Release\silicon_switch_observer_node.exe --listen 0.0.0.0 --port 8002 --name windows-laptop-2 --updates 300 --timeout-ms 30000
```

Linux observer:

```bash
./build-linux/silicon_switch_observer_node --listen 0.0.0.0 --port 8003 --name linux-laptop --updates 300 --timeout-ms 30000
```

Then run the same server command with at least 360 ticks. Every observer must
exit successfully with JSON containing `"synchronized":true`, at least one
snapshot, zero malformed messages, and a positive final revision.

## Automated single-machine equivalent

CTest also launches one server and three independent headless observers over
loopback:

```bash
ctest --test-dir build-linux -R four_process_integration --output-on-failure
```

This proves process isolation and synchronization deterministically. The
physical run additionally proves cross-platform builds, firewalls, Wi-Fi
delivery, and independent rendering.
