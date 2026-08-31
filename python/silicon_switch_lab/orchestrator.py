"""Process lifecycle, configuration, scenario, and log collection tools."""

from __future__ import annotations

import json
import subprocess
from dataclasses import dataclass, field
from pathlib import Path
from typing import Mapping, Sequence, TextIO


@dataclass(frozen=True)
class ProcessSpec:
    name: str
    command: Sequence[str]
    environment: Mapping[str, str] = field(default_factory=dict)
    working_directory: Path | None = None


@dataclass
class _ManagedProcess:
    process: subprocess.Popen[str]
    log_file: TextIO
    log_path: Path


class LabOrchestrator:
    """Owns child processes and their durable combined stdout/stderr logs."""

    def __init__(self, log_directory: Path) -> None:
        self._log_directory = log_directory
        self._processes: dict[str, _ManagedProcess] = {}

    @staticmethod
    def load_specs(configuration: Path) -> list[ProcessSpec]:
        document = json.loads(configuration.read_text(encoding="utf-8"))
        specs = []
        for item in document.get("processes", []):
            specs.append(
                ProcessSpec(
                    name=item["name"],
                    command=tuple(item["command"]),
                    environment=item.get("environment", {}),
                    working_directory=(
                        Path(item["working_directory"])
                        if "working_directory" in item
                        else None
                    ),
                )
            )
        return specs

    def start(self, spec: ProcessSpec) -> None:
        if spec.name in self._processes:
            raise ValueError(f"process already exists: {spec.name}")
        if not spec.command:
            raise ValueError("process command cannot be empty")
        self._log_directory.mkdir(parents=True, exist_ok=True)
        log_path = self._log_directory / f"{spec.name}.log"
        log_file = log_path.open("w", encoding="utf-8")
        import os

        environment = os.environ.copy()
        environment.update(spec.environment)
        try:
            process = subprocess.Popen(
                list(spec.command),
                cwd=spec.working_directory,
                env=environment,
                stdout=log_file,
                stderr=subprocess.STDOUT,
                text=True,
            )
        except Exception:
            log_file.close()
            raise
        self._processes[spec.name] = _ManagedProcess(process, log_file, log_path)

    def start_all(self, specs: Sequence[ProcessSpec]) -> None:
        try:
            for spec in specs:
                self.start(spec)
        except Exception:
            self.stop_all()
            raise

    def stop(self, name: str, timeout: float = 5.0) -> int:
        managed = self._processes.pop(name)
        if managed.process.poll() is None:
            managed.process.terminate()
            try:
                managed.process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                managed.process.kill()
                managed.process.wait()
        managed.log_file.close()
        return managed.process.returncode

    def stop_all(self) -> dict[str, int]:
        return {name: self.stop(name) for name in reversed(tuple(self._processes))}

    def status(self) -> dict[str, int | None]:
        return {name: managed.process.poll() for name, managed in self._processes.items()}

    def log_path(self, name: str) -> Path:
        return self._processes[name].log_path

    def __enter__(self) -> "LabOrchestrator":
        return self

    def __exit__(self, *_: object) -> None:
        self.stop_all()
