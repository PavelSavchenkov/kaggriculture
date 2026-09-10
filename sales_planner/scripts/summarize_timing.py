"""Check full replay agreement and summarize paired timing edits."""
import argparse
import collections
import json
from pathlib import Path

import numpy as np


def read(path):
    return [json.loads(line) for line in path.read_text().splitlines()]


def key(row):
    return row["episode"], row["seat"], row["policy"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    args = parser.parse_args()
    directory = args.directory
    financial = {key(r): r for r in read(directory / "financial.jsonl")}
    engine = read(directory / "engine.jsonl")
    originals = {key(r)[:2]: r for r in engine if r["policy"] == "original"}
    protocol = json.loads((directory / "PROTOCOL.json").read_text())
    expected_calendars = protocol.get("player_calendars", 2 * len(protocol["cases"]))
    assert len(originals) == expected_calendars, "incomplete original engine results"
    records = []
    for row in engine:
        expected = financial[key(row)]
        base = originals[key(row)[:2]]
        mismatches = []
        for field in ["cash", "rival_cash", "margin", "delay_decisions", "delayed_units", "predicted_own_gain"]:
            if row[field] != expected[field]:
                mismatches.append("financial_" + field)
        for field in ["own_faults", "rival_faults", "own_requested", "rival_requested", "own_produced", "rival_produced",
                      "own_discarded", "rival_discarded", "own_final_stock", "rival_final_stock"]:
            if row[field] != base[field]:
                mismatches.append(field)
        for field in ["own_work_differences", "rival_work_differences", "shop_differences"]:
            if row[field]:
                mismatches.append(field)
        for field in ["own_source_contract_exceptions", "rival_source_contract_exceptions"]:
            if row.get(field, 0) != base.get(field, 0):
                mismatches.append(field)
        for field in ["own_missing", "rival_missing", "own_commitment_failures", "rival_commitment_failures"]:
            if expected[field]:
                mismatches.append(field)
        if row["policy"] == "original":
            assert not mismatches, (key(row), mismatches)
            continue
        records.append({"episode": row["episode"], "seat": row["seat"], "policy": row["policy"],
                        "margin": row["margin"] - base["margin"], "cash": row["cash"] - base["cash"],
                        "rival_cash": row["rival_cash"] - base["rival_cash"], "mismatches": mismatches,
                        "decisions": row["delay_decisions"], "delayed_units": row["delayed_units"]})
    summary = []
    for policy in sorted({r["policy"] for r in records}):
        rows = [r for r in records if r["policy"] == policy]
        assert len(rows) == expected_calendars, "incomplete policy engine results"
        clusters = collections.defaultdict(list)
        for row in rows:
            clusters[row["episode"]].append([row["margin"], row["cash"], row["rival_cash"]])
        assert all(len(c) == 2 for c in clusters.values()), "expected both player calendars per episode"
        values = np.array([np.mean(clusters[e], axis=0) for e in sorted(clusters)])
        rng = np.random.default_rng(202609100034)
        means = values[rng.integers(0, len(values), size=(10000, len(values)))].mean(axis=1)
        bounds = np.quantile(means, [0.025, 0.975], axis=0)
        result = {"policy": policy, "calendars": len(rows), "episode_clusters": len(values),
                  "invalid_or_changed": sum(bool(r["mismatches"]) for r in rows),
                  "positive": sum(r["margin"] > 0 for r in rows), "negative": sum(r["margin"] < 0 for r in rows),
                  "worst": min(r["margin"] for r in rows), "best": max(r["margin"] for r in rows),
                  "decisions": sum(r["decisions"] for r in rows)}
        for i, metric in enumerate(["margin", "cash", "rival_cash"]):
            result[metric] = {"mean": float(values[:, i].mean()), "ci95": bounds[:, i].tolist()}
        summary.append(result)
    (directory / "ENGINE_CHECKS.json").write_text(json.dumps({"games": len(engine), "source_calendars": len(originals),
        "source_episodes_with_contract_exceptions": sorted({r["episode"] for r in originals.values()
            if r.get("own_source_contract_exceptions", 0) or r.get("rival_source_contract_exceptions", 0)}),
        "invalid_or_changed": [r for r in records if r["mismatches"]]}, indent=2) + "\n")
    (directory / "PAIRED_GAMES.json").write_text(json.dumps(records, indent=2) + "\n")
    (directory / "SUMMARY.json").write_text(json.dumps(summary, indent=2) + "\n")
    (directory / "NEGATIVE_GAMES.json").write_text(json.dumps(sorted([r for r in records if r["margin"] < 0], key=lambda r: r["margin"]), indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
