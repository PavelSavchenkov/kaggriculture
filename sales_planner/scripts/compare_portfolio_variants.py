"""Paired fixed-course and selection effects; seeds are the bootstrap clusters."""
import argparse
import collections
import json
import math
from pathlib import Path

import numpy as np


def read_variant(directory, protocol):
    rows = {}
    for opponent in protocol["opponents"]:
        for line in (directory / f"{opponent}.jsonl").read_text().splitlines():
            row = json.loads(line)
            key = (opponent, row["seed"], row["seat"], row["branch"])
            assert key not in rows
            rows[key] = row
    assert len(rows) == len(protocol["opponents"]) * protocol["seeds_per_opponent"] * 2 * 3
    return rows


def utility(row):
    return float(row["margin"] > 0) + 0.5 * (row["margin"] == 0)


def paired(key, control, candidate):
    return {"opponent": key[0], "seed": key[1], "seat": key[2], "branch": key[3],
            "margin": candidate["margin"] - control["margin"],
            "cash": candidate["cash"] - control["cash"],
            "rival_cash": candidate["rival_cash"] - control["rival_cash"],
            "utility_pp": 100 * (utility(candidate) - utility(control)),
            "own_faults": candidate["own_faults"] - control["own_faults"],
            "worker_days": candidate["worker_days"] - control["worker_days"],
            "produced": [a - b for a, b in zip(candidate["produced"], control["produced"], strict=True)],
            "repair_decisions": candidate["repair_decisions"], "repair_units": candidate["repair_units"],
            "control_own_forecast_failed": bool(control["own_forecast_failures"]),
            "control_margin": control["margin"], "candidate_margin": candidate["margin"]}


def describe(rows):
    if not rows:
        return {"games": 0}
    metrics = ["margin", "cash", "rival_cash", "utility_pp", "own_faults"]
    clusters = collections.defaultdict(list)
    for row in rows:
        clusters[row["seed"]].append([row[m] for m in metrics])
    # Weight by game count so unequal conditional subgroups retain their estimand.
    sums = np.array([np.sum(clusters[s], axis=0) for s in sorted(clusters)])
    counts = np.array([len(clusters[s]) for s in sorted(clusters)])
    rng = np.random.default_rng(202609100026)
    indices = rng.integers(0, len(counts), size=(10000, len(counts)))
    samples = sums[indices].sum(axis=1) / counts[indices].sum(axis=1)[:, None]
    bounds = np.quantile(samples, [0.025, 0.975], axis=0)
    result = {"games": len(rows), "seed_clusters": len(counts)}
    for i, metric in enumerate(metrics):
        result[metric] = {"mean": sum(r[metric] for r in rows) / len(rows), "ci95": bounds[:, i].tolist()}
    result.update(positive=sum(r["margin"] > 0 for r in rows), negative=sum(r["margin"] < 0 for r in rows),
                  zero=sum(r["margin"] == 0 for r in rows), worst_margin=min(r["margin"] for r in rows),
                  best_margin=max(r["margin"] for r in rows), worst_cash=min(r["cash"] for r in rows),
                  repair_games=sum(r["repair_decisions"] > 0 for r in rows),
                  worker_days_changed=sum(r["worker_days"] != 0 for r in rows),
                  production_changed=sum(any(r["produced"]) for r in rows),
                  any_product_decreased=sum(any(v < 0 for v in r["produced"]) for r in rows),
                  own_faults_improved=sum(r["own_faults"] < 0 for r in rows),
                  own_faults_worse=sum(r["own_faults"] > 0 for r in rows))
    tail = max(1, math.ceil(len(rows) * 0.1))
    for mode in ["control", "candidate"]:
        result[f"{mode}_margin_cvar10"] = float(np.mean(sorted(r[f"{mode}_margin"] for r in rows)[:tail]))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--reference", default="control")
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    control = read_variant(args.directory / args.reference, protocol)
    for variant in protocol["variants"]:
        if variant == args.reference:
            continue
        candidate = read_variant(args.directory / variant, protocol)
        assert candidate.keys() == control.keys()
        deltas = [paired(key, control[key], candidate[key]) for key in sorted(control)]
        report = {"reference_variant": args.reference, "all_fixed_branches": describe(deltas), "by_opponent": {}, "by_branch": {}, "selection": {}}
        for opponent in protocol["opponents"]:
            report["by_opponent"][opponent] = describe([r for r in deltas if r["opponent"] == opponent])
        for branch in range(3):
            report["by_branch"][branch] = describe([r for r in deltas if r["branch"] == branch])
        for label, predicate in [
            ("repair_touched", lambda r: r["repair_decisions"] > 0),
            ("control_own_forecast_feasible", lambda r: not r["control_own_forecast_failed"]),
            ("control_own_forecast_failed", lambda r: r["control_own_forecast_failed"]),
        ]:
            report[label] = describe([r for r in deltas if predicate(r)])
        for comparator in ["chosen", "demand", "old_model"]:
            selected = []
            for key in sorted(control):
                if key[3] != 0:
                    continue
                prefix = key[:3]
                a = candidate[(*prefix, candidate[key]["chosen"])]
                b = control[(*prefix, control[key][comparator])]
                selected.append(paired(key, b, a))
            report["selection"][f"candidate_chosen_vs_control_{comparator}"] = describe(selected)
        report["coverage"] = {
            name: {"own_failed_branches": sum(bool(r["own_forecast_failures"]) for r in rows.values()),
                   "rival_failed_branches": sum(bool(r["rival_forecast_failures"]) for r in rows.values()),
                   "mean_course_seconds": float(np.mean([r["seconds"] for r in rows.values()]))}
            for name, rows in [("control", control), (variant, candidate)]}
        destination = args.directory / variant
        (destination / "PAIRED_EFFECTS.json").write_text(json.dumps(report, indent=2) + "\n")
        (destination / "PAIRED_GAMES.json").write_text(json.dumps(deltas, indent=2) + "\n")
        (destination / "NEGATIVE_GAMES.json").write_text(json.dumps([r for r in deltas if r["margin"] < 0 or r["cash"] < 0 or any(v < 0 for v in r["produced"])], indent=2) + "\n")
        print(json.dumps({"variant": variant, "fixed": report["all_fixed_branches"], "selection": report["selection"], "coverage": report["coverage"]}, indent=2))


if __name__ == "__main__":
    main()
