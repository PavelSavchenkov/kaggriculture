"""Run fixed reports when each complete frozen benchmark is ready."""
import json
import os
import subprocess
import time
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/report_queue_v3"; output.mkdir(exist_ok=False)
    jobs = {
        "ordinary": {"requirements": ["runs/holdout_b_joint_reference_v3/initial_3s/STATUS.json"],
                     "commands": [["report_holdout_b_ordinary_v3.py"]]},
        "calendar": {"requirements": ["runs/holdout_b_calendar_v3_h24/initial_3s/STATUS.json", "runs/holdout_b_calendar_v3_h23/explicit_deadlines_3s/STATUS.json"],
                     "commands": [["report_holdout_b_calendars_v3.py"]]},
        "warm": {"requirements": ["runs/warm_seed_benchmark_v2/PROTOCOL.json"],
                 "commands": [["analyze_warm_guided.py", "runs/warm_seed_benchmark_v2"],
                              ["analyze_warm_seeds.py", "runs/warm_seed_benchmark_v2"],
                              ["diagnose_warm_guidance.py", "runs/warm_seed_benchmark_v2"]]},
    }
    states = {name: "waiting_for_complete_reference" for name in jobs}
    environment = dict(os.environ)
    environment["LABOR_SEARCH_MODEL_DIR"] = str(EXP / "models/search_candidate_dev_v2")
    environment["LABOR_CONTEXT_MODEL_DIR"] = str(EXP / "models/context_candidate_dev_v2")

    def run(name):
        with (output / (name + ".log")).open("w") as log:
            for command in jobs[name]["commands"]:
                subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / command[0]), *command[1:]],
                               cwd=EXP, env=environment, stdout=log, stderr=subprocess.STDOUT, check=True)

    with ThreadPoolExecutor(max_workers=3) as pool:
        pending = {}
        while True:
            for name, job in jobs.items():
                if states[name] == "waiting_for_complete_reference":
                    ready = all(json.loads((EXP / path).read_text())["status"] == "completed" for path in job["requirements"])
                    if ready:
                        pending[name] = pool.submit(run, name); states[name] = "reporting"
                        print(name, "reporting", flush=True)
                if states[name] == "reporting" and pending[name].done():
                    error = pending[name].exception()
                    states[name] = "failed" if error else "completed"
                    print(name, states[name], str(error) if error else "", flush=True)
            status = {"utc": datetime.now(timezone.utc).isoformat(), "jobs": states, "definitions": jobs}
            (output / "STATUS.json").write_text(json.dumps(status, indent=2) + "\n")
            if all(value in ["failed", "completed"] for value in states.values()): break
            time.sleep(50)
    assert all(value == "completed" for value in states.values()), states


if __name__ == "__main__":
    main()
