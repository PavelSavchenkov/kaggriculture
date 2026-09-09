"""Report the entire frozen shop panel after every child completes."""
import json
import subprocess
import time
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/warm_shop_report_queue_v1"; output.mkdir(exist_ok=False)
    protocol = EXP / "runs/warm_shop_benchmark_v1/PROTOCOL.json"
    while json.loads(protocol.read_text())["status"] != "completed":
        (output / "STATUS.json").write_text(json.dumps({"utc": datetime.now(timezone.utc).isoformat(), "status": "waiting_for_complete_panel"}, indent=2) + "\n")
        time.sleep(50)
    with (output / "report.log").open("w") as stream:
        result = subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts/analyze_warm_shops.py")],
                                cwd=EXP, stdout=stream, stderr=subprocess.STDOUT)
    (output / "STATUS.json").write_text(json.dumps({"utc": datetime.now(timezone.utc).isoformat(), "status": "completed" if result.returncode == 0 else "failed", "returncode": result.returncode}, indent=2) + "\n")
    assert result.returncode == 0


if __name__ == "__main__":
    main()
