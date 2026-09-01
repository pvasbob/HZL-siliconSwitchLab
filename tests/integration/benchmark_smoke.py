import argparse
import json
import socket
import subprocess
import time


def run_json(command: list[str]) -> dict:
    completed = subprocess.run(command, capture_output=True, text=True, timeout=10)
    if completed.returncode != 0:
        raise RuntimeError(completed.stdout + completed.stderr)
    return json.loads(completed.stdout.strip().splitlines()[-1])


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", required=True)
    args = parser.parse_args()

    reports = [
        run_json([args.benchmark, "simulation", "--backend", "cpu", "--iterations", "100"]),
        run_json([args.benchmark, "forwarding", "--iterations", "100", "--payload", "128"]),
        run_json([args.benchmark, "render", "--iterations", "100", "--nodes", "10"]),
        run_json([args.benchmark, "queue", "--iterations", "1000", "--capacity", "16"]),
    ]

    reservation = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    reservation.bind(("127.0.0.1", 0))
    port = reservation.getsockname()[1]
    reservation.close()
    server = subprocess.Popen(
        [
            args.benchmark,
            "udp-server",
            "--listen",
            "127.0.0.1",
            "--port",
            str(port),
            "--packets",
            "20",
            "--timeout-ms",
            "5000",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    try:
        time.sleep(0.1)
        client = run_json(
            [
                args.benchmark,
                "udp-client",
                "--target",
                "127.0.0.1",
                "--port",
                str(port),
                "--packets",
                "20",
                "--payload",
                "128",
                "--timeout-ms",
                "1000",
            ]
        )
        output, _ = server.communicate(timeout=10)
        if server.returncode != 0:
            raise RuntimeError(output)
        reports.extend([json.loads(output.strip().splitlines()[-1]), client])
    finally:
        if server.poll() is None:
            server.terminate()
            server.wait(timeout=5)

    if client["received"] != 20 or client["loss_percent"] != 0.0:
        raise RuntimeError(f"unexpected loopback loss: {client}")
    print(json.dumps({"benchmarks": reports}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
