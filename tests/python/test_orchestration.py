import json
import sys
import tempfile
import time
import unittest
from pathlib import Path

from silicon_switch_lab.control_client import ControlMessage, ControlMessageType
from silicon_switch_lab.orchestrator import LabOrchestrator, ProcessSpec


class ControlProtocolTests(unittest.TestCase):
    def test_round_trip(self) -> None:
        message = ControlMessage(ControlMessageType.CREATE_VLAN, 17, b"payload")
        self.assertEqual(ControlMessage.decode(message.encode()), message)

    def test_rejects_bad_magic(self) -> None:
        encoded = bytearray(
            ControlMessage(ControlMessageType.QUERY_COUNTERS, 1).encode()
        )
        encoded[0] = 0
        with self.assertRaisesRegex(ValueError, "magic"):
            ControlMessage.decode(encoded)

    def test_rejects_bad_length(self) -> None:
        encoded = ControlMessage(ControlMessageType.QUERY_COUNTERS, 1).encode() + b"x"
        with self.assertRaisesRegex(ValueError, "length"):
            ControlMessage.decode(encoded)


class OrchestratorTests(unittest.TestCase):
    def test_loads_configuration(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            config = Path(directory) / "lab.json"
            config.write_text(
                json.dumps(
                    {
                        "processes": [
                            {
                                "name": "observer",
                                "command": ["observer", "--port", "9000"],
                                "environment": {"NODE_ID": "one"},
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )
            specs = LabOrchestrator.load_specs(config)
            self.assertEqual(specs[0].name, "observer")
            self.assertEqual(tuple(specs[0].command), ("observer", "--port", "9000"))
            self.assertEqual(specs[0].environment["NODE_ID"], "one")

    def test_launches_collects_logs_and_stops(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            logs = Path(directory) / "logs"
            orchestrator = LabOrchestrator(logs)
            orchestrator.start(
                ProcessSpec(
                    "worker",
                    (
                        sys.executable,
                        "-c",
                        "import time; print('ready', flush=True); time.sleep(30)",
                    ),
                )
            )
            log_path = orchestrator.log_path("worker")
            for _ in range(50):
                if log_path.exists() and "ready" in log_path.read_text(encoding="utf-8"):
                    break
                time.sleep(0.01)
            self.assertIn("ready", log_path.read_text(encoding="utf-8"))
            self.assertIsNone(orchestrator.status()["worker"])
            orchestrator.stop_all()
            self.assertEqual(orchestrator.status(), {})


if __name__ == "__main__":
    unittest.main()
