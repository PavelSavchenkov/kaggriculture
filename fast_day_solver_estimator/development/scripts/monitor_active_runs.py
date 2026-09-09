"""Check live driver processes as well as persistent run status files."""
import json
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
JOBS = {
    "ordinary_reference": ("runs/holdout_b_joint_reference_v3/initial_3s/STATUS.json", ["python scripts/run_reference.py holdout_b_joint_reference_v3"]),
    "h24_reference": ("runs/holdout_b_calendar_v3_h24/initial_3s/STATUS.json", ["python scripts/run_reference.py holdout_b_calendar_v3_h24"]),
    "h23_reference": ("runs/holdout_b_calendar_v3_h23/explicit_deadlines_3s/STATUS.json", ["python scripts/run_reference.py holdout_b_calendar_v3_h23"]),
    "stage_benchmark": ("runs/warm_stage_seed_benchmark_v3/PROTOCOL.json", ["python scripts/benchmark_warm_stage_seeds.py", "python scripts/continue_warm_stage_benchmark.py"]),
    "shop_benchmark": ("runs/warm_shop_benchmark_v1/PROTOCOL.json", ["python scripts/benchmark_warm_shops.py"]),
    "long_retry_reference": ("runs/holdout_b_long_retry_reference_v1/long_retry_30s/STATUS.json", ["scripts/run_reference.py holdout_b_long_retry_reference_v1"]),
}


def main():
    output = EXP / "runs/active_run_monitor_v1"; output.mkdir(exist_ok=False)
    previous = {}
    deadline = datetime(2026, 9, 10, 0, 56, 39, tzinfo=timezone.utc)
    while datetime.now(timezone.utc) < deadline:
        processes = subprocess.run(["ps", "-eo", "pid=,args="], text=True, capture_output=True, check=True).stdout.splitlines()
        states = {}
        for name, (status_path, commands) in JOBS.items():
            path = EXP / status_path
            if not path.exists():
                states[name] = {"state": "not_started"}
                continue
            document = json.loads(path.read_text()); status = document["status"]
            matches = [line.split(maxsplit=1)[0] for line in processes if any(command in line for command in commands)]
            state = "completed" if status == "completed" else "running" if matches else "driver_missing_while_status_unfinished"
            states[name] = {"state": state, "stored_status": status, "driver_pids": matches}
            if state != previous.get(name):
                print(datetime.now(timezone.utc).isoformat(), name, state, flush=True)
        report = {"utc": datetime.now(timezone.utc).isoformat(), "jobs": states,
                  "unexpected_missing_drivers": [name for name, state in states.items() if state["state"] == "driver_missing_while_status_unfinished"]}
        temporary = output / "STATUS.tmp"
        temporary.write_text(json.dumps(report, indent=2) + "\n"); temporary.replace(output / "STATUS.json")
        previous = {name: state["state"] for name, state in states.items()}
        time.sleep(50)


if __name__ == "__main__":
    main()
