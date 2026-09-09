"""Wait for all stage-policy seed pairs before computing aggregate reports."""
import json
import subprocess
import time
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    root = EXP / "runs/warm_stage_seed_benchmark_v3"
    while json.loads((root / "PROTOCOL.json").read_text())["status"] != "completed": time.sleep(50)
    for script in ["analyze_warm_guided.py", "analyze_warm_seeds.py", "diagnose_warm_stage.py"]:
        subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts" / script), str(root)], cwd=EXP, check=True)


if __name__ == "__main__":
    main()
