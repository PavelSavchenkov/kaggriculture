"""Verify and save input-only adaptive planning and query forecasts."""
import argparse
import hashlib
import json
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
import pandas as pd
from threadpoolctl import threadpool_limits

from baseline_study import cost
from check_context_model import timing
from query_cost_view import curve_scores


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    for name in ["snapshot", "inputs", "query_check", "output"]: parser.add_argument(name, type=Path)
    args = parser.parse_args()
    data = json.loads((args.inputs / "INPUT_DATASET.json").read_text())
    assert data["schema"] == "unlabeled_planning_inputs_v3"
    assert all(set(q) == {"id", "workers"} for r in data["rows"] for q in r["query_outcomes"])
    report = json.loads((args.query_check / "CHECK.json").read_text())
    assert max(report["max_absolute_error"].values()) < 1e-7
    records = {r["id"]: r for r in data["rows"]}
    ordinary = {key: r["active_hours"] == 24 and r["profile"] == "earliest" and len(r["hire_slots"]) == 39 for key, r in records.items()}
    sys.path.insert(0, str((args.snapshot / "cost_study").resolve()))
    fitted = joblib.load(args.snapshot / "cost_study/models.joblib")
    assert data["feature_names"][:233] == fitted["feature_names"]
    with threadpool_limits(limits=1):
        points = fitted["models"]["cost_direct_extra"].predict(np.float32([r["features"][:233] for r in records.values()]))
    direct = dict(zip(records, points))
    queries = pd.read_csv(args.query_check / "CPP.csv")
    assert set(queries["pass"]) == {0}
    probabilities = {(r.id, r.workers): r.boost for r in queries.itertuples()}
    args.output.mkdir(exist_ok=False)
    manifest = args.query_check / "INPUT.txt"
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(args.snapshot / "build/predict_planning"), str(manifest), str(args.output / "CPP.csv"), "3"], cwd=EXP, check=True)
    actual = pd.read_csv(args.output / "CPP.csv")
    assert len(actual) == len(records) * 6 and not actual.duplicated(["id", "pass", "forced"]).any()
    max_error = 0.0
    for row in actual.itertuples():
        case = records[row.id]; lower = case["lower_bound"]; upper = len(case["hire_slots"]) + 1
        rejected = lower > upper or any(case[k] > 0 for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"])
        assert bool(row.rejected) == rejected and row.lower == lower
        if rejected:
            assert np.isnan(row.workers) and np.isnan(row.cost) and row.evaluations == 0
            continue
        assert bool(row.direct) == ordinary[row.id]
        workers = np.arange(lower, upper + 1)
        curve, peak = curve_scores(workers, [probabilities[row.id, int(k)] for k in workers])
        point = max(lower, direct[row.id]) if row.direct else curve["curve_mean_workers"]
        complete = bool(row.forced) or not row.direct or probabilities[row.id, int(np.ceil(point))] < .5
        assert bool(row.complete_curve) == complete and bool(row.low_peak) == (peak < .5)
        assert row.evaluations == (len(workers) if complete else 1)
        expected = {"workers": point, "cost": float(cost([point])[0])}
        if row.direct:
            probe = int(np.ceil(point)); pp = probabilities[row.id, probe]
            expected["probe_probability"] = pp
            assert row.probe_workers == probe and bool(row.weak_probe) == (pp < .2)
        if complete:
            expected.update(peak=peak, mean_workers=curve["curve_mean_workers"], mean_cost=float(cost([curve["curve_mean_cost"]])[0]))
            assert [row.q25, row.q50, row.q75, row.q90] == [curve[f"curve_quantile_{q:g}"] for q in [.25, .5, .75, .9]]
            assert row.absolute_half == curve["curve_absolute_half"]
        for field, value in expected.items():
            error = abs(getattr(row, field) - value); max_error = max(max_error, error)
            assert error <= 1e-7 * max(1, abs(value)), (row.id, field, getattr(row, field), value)
    predictions = []
    for row in actual[(actual["pass"] == 0) & (actual["forced"] == 0)].itertuples():
        case = records[row.id]
        predictions.append({"id": row.id, "contract_key": case["contract_key"], "obligation_key": case["obligation_key"],
            "source_ids": case["source_ids"], "method": "adaptive_planning_v3", "active_hours": case["active_hours"],
            "profile": case["profile"], "rejected": bool(row.rejected), "uses_direct_cost": bool(row.direct),
            "lower_bound": row.lower, "workers": row.workers if not row.rejected else None,
            "cost": row.cost if not row.rejected else None, "complete_curve": bool(row.complete_curve),
            "low_peak": bool(row.low_peak), "weak_probe": bool(row.weak_probe),
            "probe_workers": row.probe_workers, "probe_probability": row.probe_probability if not row.rejected and row.direct else None,
            "query_evaluations": row.evaluations, "cpu_us": row.cpu_us})
    (args.output / "PLANNING_PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True, allow_nan=False) + "\n" for r in predictions))
    forecasts = []
    for row in queries.itertuples():
        case = records[row.id]
        for key in sorted(set(case["source_ids"]) | {row.id}):
            for method in ["boost", "logistic"]:
                forecasts.append({"context_id": row.id, "query_id": f"{key}_w{row.workers:02}", "workers": row.workers,
                                  "method": method, "predicted_success": getattr(row, method), "predicted_cpu_seconds": row.cpu})
    assert len({(r["query_id"], r["method"]) for r in forecasts}) == len(forecasts)
    (args.output / "QUERY_PREDICTIONS.jsonl").write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in forecasts))
    profiles = []
    for forced in [0, 1]:
        for subset in ["all", "ordinary", "other_calendar"]:
            group = actual[actual["forced"] == forced]
            if subset != "all": group = group[group["id"].map(ordinary) == (subset == "ordinary")]
            profiles.append({"forced": forced, "subset": subset, "records": len(group), "cpu_us": timing(group["cpu_us"]),
                             "mean_query_evaluations": float(group["evaluations"].mean())})
    paths = [args.snapshot / "FREEZE.json", args.inputs / "INPUT_DATASET.json", args.query_check / "CHECK.json", args.query_check / "CPP.csv", manifest, Path(__file__)]
    result = {"utc": datetime.now(timezone.utc).isoformat(), "scope": "Prospective input-only predictions before holdout_b reference calls.",
              "contexts": len(records), "predictions_checked": len(actual), "max_absolute_error": max_error,
              "query_forecasts_including_aliases": len(forecasts), "timing": profiles,
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
              "output_sha256": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in args.output.iterdir()}}
    (args.output / "MANIFEST.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ["input_sha256", "output_sha256", "timing"]}, indent=2))


if __name__ == "__main__":
    main()
