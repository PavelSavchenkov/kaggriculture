"""Verify adaptive/full C++ curves against independently checked probabilities."""
import argparse
import hashlib
import json
import subprocess
import sys
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
    parser.add_argument("name")
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    previous = EXP / "runs/context_cpp_check_v2"
    check = json.loads((previous / "CHECK.json").read_text())
    assert check["query_checks"] == 225000 and max(check["max_absolute_error"].values()) < 1e-7
    sources = [EXP / "runs/ordinary_context_development_v2/DATASET.json", EXP / "runs/calendar_complete_initial_v2/DATASET.json"]
    records = {}
    for path in sources:
        for row in json.loads(path.read_text())["rows"]:
            if row["id"] in records:
                assert np.array_equal(np.float32(records[row["id"]]["features"]), np.float32(row["features"]))
            records[row["id"]] = row
    manifest = previous / "INPUT.txt"
    ordinary = {}
    for line in manifest.read_text().splitlines():
        key, path, hours, fixed, optional, *slots = line.split()
        assert int(fixed) == 0
        p = json.loads(Path(path).read_text())
        occupied = {(e["hour"], e["order_index"]) for e in p["buy_schedule"] if e["op"] != "hire"}
        expected = [(h, s) for h in range(23) for s in range(10) if (h, s) not in occupied][:39]
        actual = list(zip(map(int, slots[::2]), map(int, slots[1::2])))
        ordinary[key] = int(hours) == 24 and int(optional) == 39 and actual == expected
    sys.path.insert(0, str(EXP / "runs/cost_model_study_v3"))
    model_path = EXP / "runs/cost_model_study_v3/models.joblib"
    model = joblib.load(model_path)["models"]["cost_direct_extra"]
    keys = list(records)
    with threadpool_limits(limits=1):
        raw = model.predict(np.float32([records[k]["features"][:233] for k in keys]))
    direct = dict(zip(keys, raw))
    reference = pd.read_csv(previous / "CPP.csv")
    reference = reference[reference["pass"] == 0]
    probabilities = {(r.id, r.workers): r.boost for r in reference.itertuples()}
    subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                    str(EXP / "build/predict_planning"), str(manifest), str(output / "CPP.csv"), "3"], cwd=EXP, check=True)
    actual = pd.read_csv(output / "CPP.csv")
    assert len(actual) == len(records) * 6 and not actual.duplicated(["id", "pass", "forced"]).any()
    max_error = 0.0
    for row in actual.itertuples():
        case = records[row.id]; lower = case["lower_bound"]; upper = len(case["hire_slots"]) + 1
        rejected = lower > upper or any(case.get(k, 0) > 0 for k in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"])
        assert bool(row.rejected) == rejected and row.lower == lower
        if rejected:
            assert np.isnan(row.workers) and np.isnan(row.cost) and row.evaluations == 0
            continue
        assert bool(row.direct) == ordinary[row.id]
        workers = np.arange(lower, upper + 1)
        ps = [probabilities[row.id, int(k)] for k in workers]
        curve, peak = curve_scores(workers, ps)
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
            error = abs(getattr(row, field) - value)
            max_error = max(max_error, error)
            assert error <= 1e-7 * max(1, abs(value)), (row.id, field, getattr(row, field), value)
    first = actual[actual["pass"] == 0]
    profiles = []
    for forced in [0, 1]:
        for subset in ["all", "ordinary", "other_calendar"]:
            group = actual[actual["forced"] == forced]
            if subset != "all":
                group = group[group["id"].map(ordinary) == (subset == "ordinary")]
            profiles.append({"forced": forced, "subset": subset, "records": len(group), "cpu_us": timing(group["cpu_us"]),
                             "mean_query_evaluations": float(group["evaluations"].mean())})
    report = {"contexts": len(records), "predictions_checked": len(actual), "max_absolute_error": max_error,
        "adaptive_low_peak_flags": int(first[(first["forced"] == 0)]["low_peak"].sum()),
        "adaptive_ordinary_curve_fraction": float(first[(first["forced"] == 0) & (first["direct"] == 1)]["complete_curve"].mean()),
        "timing": profiles,
        "scope": "Full C++ menu/features/bounds/cost model/adaptive probability and curve path. Predictions compared with Python cost model and query probabilities previously independently checked on 225,000 calls. No fresh accuracy or complete warm-pipeline gain claim.",
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [*sources, manifest, previous / "CHECK.json", previous / "CPP.csv",
            model_path, EXP / "include/planning_estimator.hpp", EXP / "source/predict_planning.cpp", EXP / "build/predict_planning", Path(__file__)]}}
    (output / "CHECK.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
