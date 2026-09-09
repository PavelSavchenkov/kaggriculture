"""Pin all remaining input-only controls before starting third-wave solves."""
import csv
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args):
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", *map(str, args)], cwd=EXP, check=True)


def main():
    inputs = EXP / "runs/holdout_b_inputs_v3"
    snapshot = EXP / "snapshots/planning_wave_v3"
    calendar = EXP / "data/holdout_b_calendar_v3"
    references = [EXP / "runs" / name for name in ["holdout_b_joint_reference_v3", "holdout_b_calendar_v3_h24", "holdout_b_calendar_v3_h23"]]
    assert all(p.is_file() for p in [EXP / "runs/holdout_b_cost_predictions_v3/MANIFEST.json", EXP / "runs/holdout_b_planning_predictions_v3/MANIFEST.json"])
    assert all(not list(root.glob("*/STATUS.json")) for root in references), "a reference round already started"
    features = {r["id"]: r for r in csv.DictReader((inputs / "FEATURES.csv").open())}
    index = [json.loads(line) for line in (calendar / "index.jsonl").read_text().splitlines()]
    for hours in [23, 24]:
        with (calendar / f"FEATURES_{hours}.csv").open("x") as stream:
            writer = csv.DictWriter(stream, fieldnames=list(next(iter(features.values()))))
            writer.writeheader(); writer.writerows(features[r["id"]] for r in index if r["active_hours"] == hours)
    manifest = inputs / "CONTEXT_ORIGINAL.txt"
    manifest.write_text("".join(" ".join(line.split()[:2]) + "\n" for line in (inputs / "FEATURES.txt").read_text().splitlines()))
    run(EXP.parents[2] / "day_solver/with_runtime.sh", snapshot / "build/original_baseline", manifest, inputs / "CONTEXT_ORIGINAL.csv")
    run("python", "scripts/predict_calendar_baselines.py", calendar, snapshot)
    original = {r["id"]: r for r in csv.DictReader((inputs / "CONTEXT_ORIGINAL.csv").open())}
    rows = []
    for row in index:
        lower = int(float(features[row["id"]]["planning_lower_bound"]))
        point = float(original[row["id"]]["workers"])
        for method, value in [("original_geometry", point), ("original_geometry_calendar_floor", max(lower, point))]:
            rows.append({"id": row["id"], "contract_key": row["contract_key"], "method": method,
                         "raw_prediction": point, "ranking_score": value})
    with (calendar / "ORIGINAL_PREDICTIONS.jsonl").open("x") as stream:
        stream.write("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    files = [inputs / "PREPARED.json", inputs / "INPUT_DATASET.json", inputs / "NOVELTY.jsonl", inputs / "CONTEXT_ORIGINAL.csv",
             calendar / "ORIGINAL_PREDICTIONS.jsonl", calendar / "BASELINE_PREDICTIONS.jsonl", calendar / "BASELINE_FREEZE.json"]
    for name in ["holdout_b_cost_predictions_v3", "holdout_b_planning_predictions_v3", "holdout_b_context_check_v3"]:
        files.extend(p for p in (EXP / "runs" / name).iterdir() if p.is_file())
    files.extend(root / name for root in references for name in ["CASES.json", "PROTOCOL.json"])
    files.extend(EXP / "scripts" / name for name in ["prepare_planning_wave_inputs.py", "freeze_planning_predictions.py", "finish_planning_wave_freeze.py"])
    frozen = json.loads((snapshot / "FREEZE.json").read_text())
    assert all(sha(EXP / info["source"]) == info["sha256"] for info in frozen["files"].values()), "frozen candidate changed"
    result = {"utc": datetime.now(timezone.utc).isoformat(), "status": "all predictions frozen before reference calls",
              "candidate_freeze_sha256": sha(snapshot / "FREEZE.json"),
              "assembly_addendum": "The three new Python readers assemble frozen cases, verify unchanged C++ predictions and save controls. They were added after download and before solving. No candidate weights, thresholds, generator rules or query decisions changed. The older calendar baseline export retains its development wording; this outer manifest identifies its prospective application.",
              "reference_jobs": {"ordinary_joint": 6, "calendar_h24": 1, "calendar_h23": 1},
              "query_counts": {root.name: sum(len(r["queries"]) for r in json.loads((root / "CASES.json").read_text())) for root in references},
              "input_sha256": {str(p.relative_to(EXP)): sha(p) for p in files}}
    with (inputs / "PREDICTIONS_FROZEN.json").open("x") as stream: stream.write(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
