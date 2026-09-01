import argparse
import json
import socket
import subprocess
import threading
import time


def available_udp_ports(count: int) -> list[int]:
    candidates = []
    try:
        for _ in range(count):
            candidate = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            candidate.bind(("127.0.0.1", 0))
            candidates.append(candidate)
        return [candidate.getsockname()[1] for candidate in candidates]
    finally:
        for candidate in candidates:
            candidate.close()


def run_server(server: str, proxy_port: int, ticks: int) -> str:
    completed = subprocess.run(
        [
            server,
            "--backend",
            "cpu",
            "--ticks",
            str(ticks),
            "--hz",
            "200",
            "--observer",
            f"127.0.0.1:{proxy_port}",
        ],
        capture_output=True,
        text=True,
        timeout=10,
    )
    if completed.returncode != 0:
        raise RuntimeError(completed.stdout + completed.stderr)
    return completed.stdout.strip()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--server", required=True)
    parser.add_argument("--observer", required=True)
    args = parser.parse_args()

    observer_port, proxy_port = available_udp_ports(2)
    stop_proxy = threading.Event()
    proxy_ready = threading.Event()
    proxy_report = {"received": 0, "forwarded": 0, "dropped": 0}

    def lossy_proxy() -> None:
        incoming = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        outgoing = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            incoming.bind(("127.0.0.1", proxy_port))
            incoming.settimeout(0.1)
            proxy_ready.set()
            while not stop_proxy.is_set():
                try:
                    payload, _ = incoming.recvfrom(65_507)
                except socket.timeout:
                    continue
                proxy_report["received"] += 1
                if proxy_report["received"] == 5:
                    proxy_report["dropped"] += 1
                    continue
                outgoing.sendto(payload, ("127.0.0.1", observer_port))
                proxy_report["forwarded"] += 1
        finally:
            incoming.close()
            outgoing.close()

    proxy = threading.Thread(target=lossy_proxy, daemon=True)
    proxy.start()
    if not proxy_ready.wait(timeout=2):
        raise RuntimeError("lossy UDP proxy did not start")

    observer = subprocess.Popen(
        [
            args.observer,
            "--listen",
            "127.0.0.1",
            "--port",
            str(observer_port),
            "--name",
            "recovery-observer",
            "--updates",
            "45",
            "--timeout-ms",
            "10000",
        ],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    try:
        time.sleep(0.1)
        malformed = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            malformed.sendto(b"malformed-state-update", ("127.0.0.1", observer_port))
        finally:
            malformed.close()

        first_server = run_server(args.server, proxy_port, 20)
        time.sleep(0.35)  # A deliberate outage between server sessions.
        second_server = run_server(args.server, proxy_port, 41)
        stop_proxy.set()
        proxy.join(timeout=2)

        output, _ = observer.communicate(timeout=10)
        if observer.returncode != 0:
            raise RuntimeError(f"observer failed: {output}")
        report = json.loads(output.strip().splitlines()[-1])
        if not report["synchronized"] or report["revision"] != 41:
            raise RuntimeError(f"observer did not recover from restart: {report}")
        if report["snapshots"] < 2 or report["resyncs"] < 1:
            raise RuntimeError(f"loss/restart recovery was not observed: {report}")
        if report["malformed"] < 1 or proxy_report["dropped"] != 1:
            raise RuntimeError(
                f"fault injection was not observed: {report}, {proxy_report}"
            )
        print(
            json.dumps(
                {
                    "scenario": "reliability-recovery",
                    "first_server": first_server,
                    "second_server": second_server,
                    "proxy": proxy_report,
                    "observer": report,
                }
            )
        )
        return 0
    finally:
        if observer.poll() is None:
            observer.terminate()
            observer.wait(timeout=5)
        stop_proxy.set()
        proxy.join(timeout=2)


if __name__ == "__main__":
    raise SystemExit(main())
