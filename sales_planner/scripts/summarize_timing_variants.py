"""Keep all failures; pair variants on the same recorded player plans."""
import argparse
import collections
import json
from pathlib import Path

import numpy as np


def describe(rows):
    if not rows:
        return {"cases": 0}
    clusters = collections.defaultdict(list)
    for r in rows:
        clusters[r["episode"]].append([r["margin_gain"], r["cash_gain"], r["utility_gain_pp"]])
    counts = np.array([len(v) for v in clusters.values()])
    sums = np.array([np.sum(v, axis=0) for v in clusters.values()])
    rng = np.random.default_rng(202609100103)
    indices = rng.integers(0, len(counts), (10000, len(counts)))
    samples = sums[indices].sum(axis=1) / counts[indices].sum(axis=1)[:, None]
    bounds = np.quantile(samples, [0.025, 0.975], axis=0)
    report = {"cases": len(rows), "episode_clusters": len(counts),
              "positive": sum(r["margin_gain"] > 0 for r in rows), "negative": sum(r["margin_gain"] < 0 for r in rows),
              "zero": sum(r["margin_gain"] == 0 for r in rows), "worst_margin": min(r["margin_gain"] for r in rows),
              "best_margin": max(r["margin_gain"] for r in rows),
              "changed_state": sum(bool(r["changed_state"]) for r in rows),
              "history_underestimates": sum(r["history_underestimates"] for r in rows),
              "decisions": sum(r["decisions"] for r in rows)}
    for i, metric in enumerate(["margin_gain", "cash_gain", "utility_gain_pp"]):
        report[metric] = {"mean": sum(r[metric] for r in rows) / len(rows), "ci95": bounds[:, i].tolist()}
    return report


def utility(row):
    return float(row["margin"] > 0) + 0.5 * (row["margin"] == 0)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    protocol = json.loads((args.directory / "PROTOCOL.json").read_text())
    statuses = json.loads((args.directory / "RUN_STATUS.json").read_text())
    rows, failures, raw = [], [], {}
    originals = {}
    for status in statuses:
        path = args.directory / status["variant"] / f"{status['episode']}_{status['seat']}.jsonl"
        values = [json.loads(line) for line in path.read_text().splitlines()]
        if status["returncode"]:
            failures.append(status)
            continue
        assert len(values) == 2 and values[0]["policy"] == "original" and values[1]["policy"] == status["variant"]
        old, new = values
        original_key = old["episode"], old["seat"]
        old_signature = {k: v for k, v in old.items() if k not in ["seconds", "history_underestimates"]}
        if original_key in originals:
            assert originals[original_key] == old_signature
        originals[original_key] = old_signature
        changed = [k for k in ["own_faults", "rival_faults", "own_requested", "rival_requested", "own_produced", "rival_produced",
                              "own_discarded", "rival_discarded", "own_final_stock", "rival_final_stock"] if old[k] != new[k]]
        changed += [k for k in ["own_work_differences", "rival_work_differences", "shop_differences"] if new[k]]
        row = {k: status[k] for k in ["variant", "source", "episode", "seat"]} | {
            "margin_gain": new["margin"] - old["margin"], "cash_gain": new["cash"] - old["cash"],
            "rival_cash_gain": new["rival_cash"] - old["rival_cash"], "utility_gain_pp": 100 * (utility(new) - utility(old)),
            "changed_state": changed, "history_underestimates": new["history_underestimates"],
            "decisions": new["delay_decisions"], "delayed_units": new["delayed_units"]}
        rows.append(row)
        raw[status["variant"], *original_key] = new
    summaries = []
    comparisons = []
    for variant in protocol["variants"]:
        for source in ["all", "old", "fresh"]:
            selected = [r for r in rows if r["variant"] == variant and (source == "all" or r["source"] == source)]
            failed = [s for s in failures if s["variant"] == variant and (source == "all" or s["source"] == source)]
            summaries.append({"variant": variant, "source": source, "failed": len(failed),
                              "quality_scope": "Completed cases only; not an all-case promotion result if failures or state changes exist.", **describe(selected)})
            if variant == "wait_output_recent":
                continue
            paired = []
            for row in selected:
                reference = raw.get(("wait_output_recent", row["episode"], row["seat"]))
                if reference is None:
                    continue
                candidate = raw[variant, row["episode"], row["seat"]]
                paired.append(row | {"margin_gain": candidate["margin"] - reference["margin"],
                                     "cash_gain": candidate["cash"] - reference["cash"],
                                     "utility_gain_pp": 100 * (utility(candidate) - utility(reference))})
            comparisons.append({"variant": variant, "reference": "wait_output_recent", "source": source, **describe(paired)})
    for name, data in [("SUMMARY", summaries), ("COMPARISONS", comparisons), ("PAIRED_GAMES", rows), ("FAILURES", failures),
                       ("NEGATIVE_OR_CHANGED", [r for r in rows if r["margin_gain"] < 0 or r["changed_state"] or r["history_underestimates"]])]:
        (args.directory / (name + ".json")).write_text(json.dumps(data, indent=2) + "\n")
    print(json.dumps({"summaries": summaries, "comparisons": comparisons}, indent=2))


if __name__ == "__main__":
    main()
