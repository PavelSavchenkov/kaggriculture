"""Stop this session's identified process groups at the user's finalization request."""
import json
import os
import signal
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
GROUPS = {
    1195976: "run_reference.py holdout_b_joint_reference_v3",
    1195984: "run_reference.py holdout_b_calendar_v3_h23",
    1270585: "watch_planning_reports_v3.py",
    1304264: "watch_warm_stage_reports.py",
    1401300: "watch_long_retry_holdout.py",
    1406790: "continue_warm_stage_benchmark.py",
    1423647: "monitor_active_runs.py",
    1424339: "benchmark_warm_shops.py",
    1424423: "watch_warm_shop_report.py",
    1440492: "watch_long_retry_report.py",
}


def processes():
    return subprocess.check_output(["ps", "-eo", "pid,ppid,pgid,stat,etimes,args"], text=True)


def main():
    output = EXP / "evidence/finalization"
    output.mkdir(exist_ok=False)
    before = processes()
    (output / "PROCESSES_BEFORE.txt").write_text(before)
    groups = []
    for pgid, expected in GROUPS.items():
        path = Path(f"/proc/{pgid}/cmdline")
        if not path.exists():
            continue
        command = path.read_bytes().replace(b"\0", b" ").decode()
        assert expected in command and os.getpgid(pgid) == pgid and pgid != os.getpgrp(), (pgid, command)
        groups.append({"pgid": pgid, "command": command})
    report = {"requested_utc": datetime.now(timezone.utc).isoformat(),
              "reason": "User requested final packaging and then quitting, replacing the remaining timed research session.",
              "groups": groups, "semantics": "Completed evidence is retained. In-flight calls are interrupted by user request, not counted as solver failures or completed benchmark observations."}
    (output / "STOP_REQUEST.json").write_text(json.dumps(report, indent=2) + "\n")
    for group in groups:
        os.killpg(group["pgid"], signal.SIGTERM)
    time.sleep(2)
    after = processes()
    active = []
    for line in after.splitlines()[1:]:
        parts = line.split(maxsplit=5)
        if int(parts[2]) in GROUPS and not parts[3].startswith("Z"):
            active.append(line)
    assert not active, active
    (output / "PROCESSES_AFTER.txt").write_text(after)
    report["completed_utc"] = datetime.now(timezone.utc).isoformat()
    report["active_processes_remaining"] = active
    (output / "STOPPED.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"stopped_groups": len(groups), "active_processes_remaining": active}))


if __name__ == "__main__":
    main()
