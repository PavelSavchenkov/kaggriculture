"""Summarize exact C++ results and observed branch activations; no policy/search."""
import argparse
import collections
import csv
import json
import statistics
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    data = [json.loads(p.read_text()) for p in sorted((args.run / "exact").glob("*.json"))]
    lookup = {(d["agent_a"], d["agent_b"]): d for d in data}
    choices = collections.defaultdict(collections.Counter)
    for row in csv.DictReader((args.run / "choices.csv").open()):
        choices[(row["candidate"], row["opponent"])][row["chosen"]] += 1
    rows = []
    for d in data:
        name, opponent = d["agent_a"], d["agent_b"]
        parent = "shop_herd_s0_m3_g1" if name.startswith("shop_herd") else "parent"
        p = lookup[(parent, opponent)]
        baseline = {(g["seed"], g["seat"]): g for g in p["games"]}
        cash, margin, wins, faults = [], [], [], []
        changed = []
        for g in d["games"]:
            b = baseline[(g["seed"], g["seat"])]
            cash.append(g["cash"] - b["cash"])
            margin.append(g["cash"] - g["opponent_cash"] - b["cash"] + b["opponent_cash"])
            faults.append(g["unit_faults"] - b["unit_faults"])
            wins.append((g["cash"] > g["opponent_cash"]) + .5 * (g["cash"] == g["opponent_cash"]))
            changed.append(g["action_hash"] != b["action_hash"])
        row = {"candidate": name, "opponent": opponent, "games": len(cash),
               "win_utility": statistics.mean(wins), "win_change": statistics.mean(wins) - p["win_utility"],
               "cash_change": statistics.mean(cash), "margin_change": statistics.mean(margin),
               "margin_cvar10": d["margin_cvar10"], "fault_change": statistics.mean(faults),
               "changed_games": sum(changed), "branches": dict(choices[(name, opponent)])}
        rows.append(row)
    (args.run / "paired_summary.json").write_text(json.dumps(rows, indent=2) + "\n")
    with (args.run / "paired_summary.csv").open("w") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    grouped = collections.defaultdict(list)
    for row in rows:
        grouped[row["candidate"]].append(row)
    for name, values in sorted(grouped.items()):
        print(name, "mean margin change", round(statistics.mean(v["margin_change"] for v in values), 2),
              "minimum matchup change", round(min(v["margin_change"] for v in values), 2),
              "minimum win change", round(min(v["win_change"] for v in values), 4))


if __name__ == "__main__":
    main()
