"""Measure fixed-budget layout selection with complete source-family exclusion."""
import argparse
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np
from threadpoolctl import threadpool_limits

from baseline_study import cost
from models import candidates
from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("training", type=Path)
    parser.add_argument("layouts", type=Path)
    parser.add_argument("name")
    parser.add_argument("--frozen-predictions", type=Path)
    parser.add_argument("--prediction-scope", choices=["new_wave", "development_folds"], default="new_wave")
    args = parser.parse_args()
    training = json.loads(args.training.read_text()); layout = json.loads(args.layouts.read_text())
    assert training["feature_names"] == layout["feature_names"]
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    names = training["feature_names"]; ix = {name: i for i, name in enumerate(names)}
    frozen = defaultdict(dict)
    if args.frozen_predictions:
        for row in map(json.loads, args.frozen_predictions.read_text().splitlines()):
            frozen[row["method"]][row["id"]] = row["ranking_score"]
    train_rows = [r for r in training["rows"] if r["reference_workers"] is not None and len(r["families"]) == 1]
    train_x = np.array([r["features"] for r in train_rows]); train_y = np.array([r["reference_workers"] for r in train_rows])
    train_family = np.array([r["families"][0] for r in train_rows])
    pools = defaultdict(list)
    for row in layout["rows"]:
        pools[row["pool"]].append(row)
    complete = {k: v for k, v in pools.items() if len(v) == 5}
    decisions, predicted = [], []
    for family in sorted({r["families"][0] for rows in complete.values() for r in rows}):
        keep = train_family != family
        test = [r for rows in complete.values() for r in rows if r["families"] == [family]]
        x = np.array([r["features"] for r in test]); bound = np.array([r["lower_bound"] for r in test])
        models = candidates(names); values = {}
        if frozen:
            values = {name: np.array([scores[r["id"]] for r in test]) for name, scores in frozen.items()}
        else:
            with threadpool_limits(limits=1):
                for name, model in models.items():
                    model.fit(train_x[keep], train_y[keep])
                    values[name] = np.clip(np.maximum(bound, model.predict(x)), 1, 40)
        values.update({"bound": bound, "route_open": x[:, ix["route_pack_open"]],
                       "route_return": x[:, ix["route_pack_return"]], "task_distance": x[:, ix["task_distance"]]})
        # Flat additive labor assigns identical labor differences to these
        # same-work layouts; preserve that original pipeline behavior exactly.
        values["original_flat_10"] = 10 * x[:, ix["tasks"]]
        if all(r.get("original_geometry_workers") is not None for r in test):
            values["original_geometry"] = np.array([r["original_geometry_workers"] for r in test])
        for name, scores in values.items():
            by_pool = defaultdict(list)
            for row, value in zip(test, scores):
                candidate = {"id": row["id"], "pool": row["pool"], "variant": row["variant"], "family": family,
                             "method": name, "ranking_score": float(value), "screened": screen_reason(row) is not None,
                             "screen_reason": screen_reason(row),
                             "workers": row["short_sweep_workers"], "cpu_seconds": row["actual_cpu_seconds"],
                             "nominal_seconds": row["nominal_budget_seconds"]}
                by_pool[row["pool"]].append(candidate); predicted.append(candidate)
            for pool_id, rows in by_pool.items():
                known = [r for r in rows if r["workers"] is not None]
                if not known:
                    continue
                assert len({r["nominal_seconds"] for r in rows}) == 1
                oracle = min(float(cost([r["workers"]])[0]) for r in known)
                ordered = sorted((r for r in rows if not r["screened"]), key=lambda r: (
                    r["ranking_score"], hashlib.sha256(("layout_tie_v0:" + r["id"]).encode()).hexdigest()))
                original = next(r for r in rows if r["variant"] == "original")
                for mode in ["cold", "incumbent"]:
                    if mode == "incumbent" and original["workers"] is None:
                        continue
                    choices = ordered if mode == "cold" else [r for r in ordered if r["variant"] != "original"]
                    baseline_cost = float(cost([original["workers"]])[0]) if original["workers"] is not None else None
                    for budget in [1, 2, 3]:
                        selected = choices[:budget]
                        found = [float(cost([r["workers"]])[0]) for r in selected if r["workers"] is not None]
                        if mode == "incumbent":
                            found.append(baseline_cost)
                        best = min(found) if found else None
                        decisions.append({"pool": pool_id, "family": family, "method": name, "mode": mode,
                                          "candidate_budget": budget, "selected": [r["id"] for r in selected],
                                          "cost": best, "oracle_cost": oracle, "regret": best - oracle if best is not None else None,
                                          "baseline_cost": baseline_cost,
                                          "nominal_budget_seconds": budget * rows[0]["nominal_seconds"],
                                          "actual_cpu_seconds": sum(r["cpu_seconds"] for r in selected),
                                          "unsuccessful_compilations": sum(r["workers"] is None for r in selected)})
        print(family, len(test), "layout predictions complete", flush=True)
    groups = defaultdict(list)
    for row in decisions:
        groups[(row["method"], row["mode"], row["candidate_budget"])].append(row)
    summary = []
    for (method, mode, budget), rows in sorted(groups.items()):
        known = [r for r in rows if r["cost"] is not None]
        summary.append({"method": method, "mode": mode, "candidate_budget": budget, "pools": len(rows),
                        "no_certificate_rate": 1 - len(known) / len(rows),
                        "mean_verified_cost": float(np.mean([r["cost"] for r in known])) if known else None,
                        "mean_conditional_regret": float(np.mean([r["regret"] for r in known])) if known else None,
                        "mean_actual_cpu_seconds": float(np.mean([r["actual_cpu_seconds"] for r in rows])),
                        "best_cost_recall": sum(r["regret"] == 0 for r in known) / len(rows)})
    report = {"scope": "Frozen model evaluation on new families." if frozen and args.prediction_scope == "new_wave" else "Exposed development layout proxy. Hold out the entire parent family from all source and derived training contracts.",
              "frozen_predictions_sha256": hashlib.sha256(args.frozen_predictions.read_bytes()).hexdigest() if args.frozen_predictions else None,
              "complete_pools": len(complete), "partial_pools": len(pools) - len(complete),
              "training_sha256": hashlib.sha256(args.training.read_bytes()).hexdigest(),
              "layout_sha256": hashlib.sha256(args.layouts.read_bytes()).hexdigest(), "summary": summary}
    (output / "RESULTS.json").write_text(json.dumps(report, indent=2) + "\n")
    for name, rows in [("DECISIONS", decisions), ("PREDICTIONS", predicted)]:
        (output / (name + ".jsonl")).write_text("".join(json.dumps(r, sort_keys=True) + "\n" for r in rows))
    for r in summary:
        if r["candidate_budget"] == 1:
            print(r["mode"], r["method"], "cost", r["mean_verified_cost"], "fail", r["no_certificate_rate"],
                  "regret", r["mean_conditional_regret"], flush=True)


if __name__ == "__main__":
    main()
