"""Save input-only calendar-transfer controls before development labels."""
import argparse
import csv
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

import joblib
import numpy as np
from threadpoolctl import threadpool_limits


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("data", type=Path)
    parser.add_argument("snapshot", type=Path)
    args = parser.parse_args()
    sys.path.insert(0, str((args.snapshot / "cost_study").resolve()))
    fit = joblib.load(args.snapshot / "cost_study/models.joblib")
    names = fit["feature_names"]
    index = [json.loads(line) for line in (args.data / "index.jsonl").read_text().splitlines()]
    features = {r["id"]: r for hours in [23, 24] for r in csv.DictReader((args.data / f"FEATURES_{hours}.csv").open())}
    ordinary = [r for r in index if r["active_hours"] == 24]
    x = np.array([[float(features[r["id"]][name]) for name in names] for r in ordinary])
    with threadpool_limits(limits=1):
        values = fit["models"]["cost_direct_extra"].predict(x)
    forest = dict(zip([r["id"] for r in ordinary], map(float, values)))
    records = []
    for row in index:
        f, hours = features[row["id"]], row["active_hours"]
        capacity = np.cumsum([hours] + [hours - h - 1 for h, _ in row["hire_slots"]])
        lower = int(float(f["planning_lower_bound"]))

        def transfer(work):
            return 41.0 if work > capacity[-1] else max(float(lower), float(np.interp(work, capacity, np.arange(1, len(capacity) + 1))))

        forecasts = {"necessary_lower": float(lower),
                     "route_capacity_transfer": transfer(hours + (max(1, float(f["route_pack_open"])) - 1) * (hours - 1))}
        if row["id"] in forest:
            forecasts["direct_ignores_calendar"] = max(float(lower), forest[row["id"]])
            forecasts["direct_capacity_transfer"] = transfer(24 + (max(1, forest[row["id"]]) - 1) * 23)
        for method, prediction in forecasts.items():
            records.append({"id": row["id"], "contract_key": row["contract_key"], "method": method,
                            "raw_prediction": prediction, "ranking_score": prediction})
    target = args.data / "BASELINE_PREDICTIONS.jsonl"
    with target.open("x") as stream:
        stream.write("".join(json.dumps(r, sort_keys=True) + "\n" for r in records))
    paths = [args.data / "index.jsonl", *(args.data / f"FEATURES_{h}.csv" for h in [23, 24]), args.snapshot / "FREEZE.json", target, Path(__file__)]
    report = {"utc": datetime.now(timezone.utc).isoformat(), "predictions": len(records),
              "scope": "Development input-only controls. The existing cost model is used only at H24. Transfer treats its estimated earliest-menu workforce as 24+23*(k-1) work units, then inverts actual menu capacity. This is a heuristic, not a feasibility claim.",
              "unsupported": "The frozen model has no H23 support. Only analytical and route-capacity controls are emitted there.",
              "outside_menu": "41 is a workforce-overflow marker, not an unlimited-workforce infeasibility proof.",
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (args.data / "BASELINE_FREEZE.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
