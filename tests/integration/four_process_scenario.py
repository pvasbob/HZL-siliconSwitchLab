import argparse
import json
import socket
import subprocess
import time


def available_udp_ports(count: int) -> list[int]:
    sockets = []
    try:
        for _ in range(count):
            candidate = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            candidate.bind(("127.0.0.1", 0))
            sockets.append(candidate)
        return [candidate.getsockname()[1] for candidate in sockets]
    finally:
        for candidate in sockets:
            candidate.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", required=True)
    parser.add_argument("--observer", required=True)
    args = parser.parse_args()

    ports = available_udp_ports(3)
    observers = []
    for index, port in enumerate(ports, start=1):
        observers.append(
            subprocess.Popen(
                [
                    args.observer,
                    "--listen",
                    "127.0.0.1",
                    "--port",
                    str(port),
                    "--name",
                    f"observer-{index}",
                    "--updates",
                    "20",
                    "--timeout-ms",
                    "5000",
                ],
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
            )
        )

    try:
        time.sleep(0.25)
        command = [args.server, "--backend", "cpu", "--ticks", "100", "--hz", "200"]
        for port in ports:
            command.extend(["--observer", f"127.0.0.1:{port}"])
        server = subprocess.run(command, capture_output=True, text=True, timeout=10)
        if server.returncode != 0:
            raise RuntimeError(f"server failed: {server.stdout}{server.stderr}")

        reports = []
        for observer in observers:
            output, _ = observer.communicate(timeout=10)
            if observer.returncode != 0:
                raise RuntimeError(f"observer failed: {output}")
            report = json.loads(output.strip().splitlines()[-1])
            if not report["synchronized"] or report["snapshots"] < 1:
                raise RuntimeError(f"observer did not synchronize: {report}")
            reports.append(report)
        revisions = {report["revision"] for report in reports}
        if len(revisions) != 1:
            raise RuntimeError(f"observer revisions diverged: {reports}")
        print(json.dumps({"server": server.stdout.strip(), "observers": reports}))
        return 0
    finally:
        for observer in observers:
            if observer.poll() is None:
                observer.terminate()
                observer.wait(timeout=5)


if __name__ == "__main__":
    raise SystemExit(main())
