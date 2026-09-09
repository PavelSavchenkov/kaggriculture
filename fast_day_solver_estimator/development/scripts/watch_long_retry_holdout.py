"""Wait for complete short reports, then collect the frozen long-retry panel."""
import json
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/long_retry_holdout_queue_v1"; output.mkdir(exist_ok=False)
    requirements = ["runs/holdout_b_joint_reference_v3/initial_3s/STATUS.json",
                    "runs/holdout_b_calendar_v3_h24/initial_3s/STATUS.json",
                    "runs/holdout_b_calendar_v3_h23/explicit_deadlines_3s/STATUS.json"]
    while True:
        queue = json.loads((EXP / "runs/report_queue_v3/STATUS.json").read_text())["jobs"]
        assert queue["ordinary"] != "failed", "repair the ordinary report before retry assembly"
        ready = queue["ordinary"] == "completed" and all(json.loads((EXP / p).read_text())["status"] == "completed" for p in requirements)
        (output / "STATUS.json").write_text(json.dumps({"utc": datetime.now(timezone.utc).isoformat(), "status": "preparing" if ready else "waiting_for_complete_short_references_and_ordinary_reports"}, indent=2) + "\n")
        if ready:
            break
        time.sleep(50)
    commands = [["prepare_long_retry_holdout.py"], ["run_reference.py", "holdout_b_long_retry_reference_v1", "--jobs", "4", "--seconds", "30", "--round", "long_retry_30s"]]
    with (output / "run.log").open("w") as stream:
        for command in commands:
            subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / command[0]), *command[1:]], cwd=EXP, stdout=stream, stderr=subprocess.STDOUT, check=True)
    (output / "STATUS.json").write_text(json.dumps({"utc": datetime.now(timezone.utc).isoformat(), "status": "long_reference_completed_report_pending"}, indent=2) + "\n")


if __name__ == "__main__":
    main()
