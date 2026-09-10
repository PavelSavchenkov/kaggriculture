"""Paired full-game finance, state and total policy-cost summaries."""
import argparse
import json
import math
import statistics
from collections import defaultdict
from pathlib import Path

import numpy as np


def utility(row):
    return float(row["margin"] > 0) + 0.5 * (row["margin"] == 0)


def describe(rows):
    if not rows:
        return {"cases": 0}
    clusters = defaultdict(list)
    for row in rows:
        clusters[row["seed"]].append([row["margin_gain"], row["cash_gain"], row["utility_gain_pp"]])
    sums = np.array([np.sum(group, axis=0) for group in clusters.values()])
    counts = np.array([len(group) for group in clusters.values()])
    rng = np.random.default_rng(202609100115)
    sampled = rng.integers(len(clusters), size=(10000, len(clusters)))
    boot = sums[sampled].sum(axis=1) / counts[sampled].sum(axis=1)[:, None]
    result = {"cases": len(rows), "seed_clusters": len(clusters)}
    for index, key in enumerate(("margin_gain", "cash_gain", "utility_gain_pp")):
        result[key] = {"mean": statistics.mean(row[key] for row in rows),
                       "ci95": np.quantile(boot[:, index], [0.025, 0.975]).tolist()}
    result.update(positive=sum(r["margin_gain"] > 0 for r in rows),
                  negative=sum(r["margin_gain"] < 0 for r in rows),
                  zero=sum(r["margin_gain"] == 0 for r in rows),
                  worst_margin_gain=min(r["margin_gain"] for r in rows),
                  best_margin_gain=max(r["margin_gain"] for r in rows),
                  decisions=sum(r["history"]["decisions"] for r in rows),
                  history_underestimates=sum(r["history"]["history_underestimates"] for r in rows),
                  own_state_changed=sum(r["history"]["own_changed_turns"] > 0 for r in rows),
                  rival_state_changed=sum(r["history"]["rival_changed_turns"] > 0 for r in rows),
                  extra_own_faults=sum(r["history"]["own_faults"] > r["repair"]["own_faults"] for r in rows),
                  extra_rival_faults=sum(r["history"]["rival_faults"] > r["repair"]["rival_faults"] for r in rows))
    for key in ("produced", "rival_produced", "stock", "rival_stock", "discarded", "rival_discarded", "worker_days"):
        result[key + "_changes"] = sum(r["history"][key] != r["repair"][key] for r in rows)
    for variant in ("repair", "history"):
        result[variant] = {"mean_margin": statistics.mean(r[variant]["margin"] for r in rows),
                           "worst_decile_margin": statistics.mean(sorted(r[variant]["margin"] for r in rows)[:math.ceil(len(rows) / 10)]),
                           "mean_act_ms_per_game": 1000 * statistics.mean(r[variant]["act_seconds"] for r in rows),
                           "mean_compile_ms": 1000 * statistics.mean(r[variant]["compile_seconds"] for r in rows),
                           "incomplete_calendars": sum(not r[variant]["compiled"] for r in rows)}
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    statuses = json.loads((args.directory / "RUN_STATUS.json").read_text())
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    paired, failures = [], []
    for status in statuses:
        if status["exit_code"]:
            failures.append(status)
            continue
        rows = [json.loads(line) for line in (args.directory / status["output"]).read_text().splitlines()]
        assert len(rows) == 12 * protocol["seeds_per_opponent_panel"], status
        grouped = defaultdict(dict)
        for row in rows:
            key = row["seed"], row["seat"], row["branch"]
            assert row["variant"] not in grouped[key]
            grouped[key][row["variant"]] = row
        for (seed, seat, branch), pair in grouped.items():
            a, b = pair["repair"], pair["history"]
            paired.append({"seed": seed, "seat": seat, "branch": branch,
                           "panel": status["panel"], "opponent": status["opponent"],
                           "margin_gain": b["margin"] - a["margin"], "cash_gain": b["cash"] - a["cash"],
                           "utility_gain_pp": 100 * (utility(b) - utility(a)), **pair})
    report = {"all": describe(paired), "failed_jobs": len(failures), "by": {}}
    for key in ("panel", "opponent", "branch"):
        report["by"][key] = {str(value): describe([r for r in paired if r[key] == value])
                             for value in sorted({r[key] for r in paired})}
    for name, data in (("SUMMARY.json", report), ("PAIRED_GAMES.json", paired), ("FAILURES.json", failures),
                       ("NEGATIVE_OR_CHANGED.json", [r for r in paired if r["margin_gain"] < 0 or
                           r["history"]["own_changed_turns"] or r["repair"]["stock"] != r["history"]["stock"]])):
        (args.directory / name).write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps({"all": report["all"], "failed_jobs": len(failures)}, indent=2))


if __name__ == "__main__":
    main()
