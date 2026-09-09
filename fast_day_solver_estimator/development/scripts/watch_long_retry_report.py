"""Run the fixed retry reader after the entire thirty-second panel completes."""
import json
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/long_retry_report_queue_v1"; output.mkdir(exist_ok=False)
    status = EXP / "runs/holdout_b_long_retry_reference_v1/long_retry_30s/STATUS.json"
    while not status.exists() or json.loads(status.read_text())["status"] != "completed":
        (output / "STATUS.json").write_text(json.dumps({"utc": datetime.now(timezone.utc).isoformat(), "status": "waiting_for_complete_long_reference"}, indent=2) + "\n")
        time.sleep(50)
    with (output / "report.log").open("w") as stream:
        result = subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts/report_long_retry_holdout.py")],
                                cwd=EXP, stdout=stream, stderr=subprocess.STDOUT)
    (output / "STATUS.json").write_text(json.dumps({"utc": datetime.now(timezone.utc).isoformat(), "status": "completed" if result.returncode == 0 else "failed", "returncode": result.returncode}, indent=2) + "\n")
    assert result.returncode == 0


if __name__ == "__main__":
    main()
