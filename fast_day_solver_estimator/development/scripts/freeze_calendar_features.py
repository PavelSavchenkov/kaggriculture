"""Preserve a reproducible feature implementation after fixed-context extension."""
import csv
import hashlib
import json
import shutil
import subprocess
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    data = EXP / "data/calendar_development_v1"
    output = data / "reproducibility_addendum"; output.mkdir(exist_ok=False)
    files = ["source/planning_tool.cpp", "source/check_planning.cpp", "include/planning_features.hpp", "include/planning_context.hpp",
             "include/features.hpp", "include/bounds.hpp", "include/supply_bounds.hpp", "include/release_bounds.hpp",
             "build/planning_tool", "build/check_planning", "scripts/make_calendar_cases.py", "scripts/predict_calendar_baselines.py",
             "scripts/freeze_calendar_features.py", "CMakeLists.txt"]
    hashes = {}
    for name in files:
        path = EXP / name; copied = output / name; copied.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, copied); hashes[name] = hashlib.sha256(copied.read_bytes()).hexdigest()
    checks = []
    for hours in [23, 24]:
        target = output / f"FEATURES_{hours}.csv"
        subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                        str(EXP / "build/planning_tool"), str(data / f"FEATURES_{hours}.txt"), str(target)], check=True)
        before = list(csv.DictReader((data / f"FEATURES_{hours}.csv").open()))
        after = list(csv.DictReader(target.open()))
        assert len(before) == len(after)
        for a, b in zip(before, after):
            assert a.keys() == b.keys()
            assert all(a[k] == b[k] for k in a if k != "extraction_us"), a["id"]
        checks.append({"hours": hours, "contracts": len(before), "feature_columns": len(before[0]) - 2, "parity": "exact decimal values"})
    report = {"utc": datetime.now(timezone.utc).isoformat(),
              "timing": "Captured after calendar reference calls started. Fixed-crew context was added after the original 289-feature CSV files and baseline forecasts were saved. This snapshot exactly reproduces every original feature value; extraction timings differ and are excluded from parity.",
              "change_to_frozen_forecasts": "None. Original feature CSVs and 560 baseline predictions are untouched.",
              "checks": checks, "sha256": hashes}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "sha256"}, indent=2))


if __name__ == "__main__":
    main()
