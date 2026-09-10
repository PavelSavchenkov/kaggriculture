"""Dependency-free progress reporting for long-running command-line jobs."""

from __future__ import annotations

import time


class Progress:
    """Print bounded progress, throughput, elapsed time, and ETA.

    Output is line-oriented instead of terminal-control based, so progress remains
    visible in Conda, Codex, log files, notebooks, and detached training sessions.
    """

    def __init__(self, label: str, total: int, *, interval_seconds: float = 5.0):
        self.label = label
        self.total = max(int(total), 0)
        self.interval_seconds = interval_seconds
        self.started = time.monotonic()
        self.last_printed = self.started
        self.completed = 0
        self._print("starting")

    def _print(self, detail: str | None = None) -> None:
        now = time.monotonic()
        elapsed = now - self.started
        rate = self.completed / elapsed if elapsed > 0 else 0.0
        if self.total:
            percent = 100.0 * self.completed / self.total
            remaining = max(self.total - self.completed, 0)
            eta = remaining / rate if rate > 0 else float("inf")
            eta_text = f" eta={eta:.1f}s" if eta != float("inf") else ""
            status = f"{self.completed}/{self.total} ({percent:5.1f}%)"
        else:
            eta_text = ""
            status = str(self.completed)
        suffix = f" {detail}" if detail else ""
        print(f"[{self.label}] {status} elapsed={elapsed:.1f}s "
              f"rate={rate:.2f}/s{eta_text}{suffix}", flush=True)
        self.last_printed = now

    def update(self, completed: int | None = None, *, advance: int = 1,
               detail: str | None = None, force: bool = False) -> None:
        self.completed = self.completed + advance if completed is None else int(completed)
        now = time.monotonic()
        finished = self.total > 0 and self.completed >= self.total
        if force or finished or now - self.last_printed >= self.interval_seconds:
            self._print(detail)
