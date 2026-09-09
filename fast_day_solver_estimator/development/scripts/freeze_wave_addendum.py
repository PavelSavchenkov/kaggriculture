"""Record reproducibility additions without changing the frozen model selection."""
import hashlib
import json
import shutil
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    snapshot = EXP / "snapshots/search_wave_v2"
    frozen = json.loads((snapshot / "FREEZE.json").read_text())
    for name, record in frozen["files"].items():
        assert digest(snapshot / name) == record["sha256"], name
    files = ["include/original_labor.hpp", "include/query_policy.hpp", "source/predict_search.cpp",
             "source/check_query_policy.cpp", "scripts/check_query_policy.py", "scripts/freeze_search_predictions.py",
             "scripts/join_indices.py", "scripts/build_dataset.py", "scripts/check_search_cpp.py",
             "scripts/overlap_audit.py", "build/predict_search"]
    output = snapshot / "reproducibility_addendum"
    output.mkdir(exist_ok=False)
    for name in files:
        target = output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(EXP / name, target)
    report = {"utc": datetime.now(timezone.utc).isoformat(),
        "reason": "Complete original adapter header closure, record the parity-tested C++ policy port and input-only prediction writer. The prediction tool now optionally applies the already frozen supply bound and writes 17-digit doubles.",
        "unchanged": "All original freeze hashes verified. No model weights, primary policy, bounds, input selection or acceptance gate changed.",
        "reference_outcomes": "No holdout_a reference calls have been launched.",
        "files": {name: digest(output / name) for name in files}}
    (output / "MANIFEST.json").write_text(json.dumps(report, indent=2) + "\n")
    assert digest(EXP / "build/reference") == frozen["files"]["reference/reference"]["sha256"]
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
