"""Compare complete C++ outcomes with their common-seed source control."""
import argparse
import csv
import json
from pathlib import Path
from statistics import mean


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    run = args.run.resolve()
    parent = json.loads((run / "exact/parent.json").read_text())
    source = parent["games"]
    records = []
    for path in sorted((run / "exact").glob(run.name + "_*.json")):
        data = json.loads(path.read_text())
        games = data["games"]
        assert [(g["seed"], g["seat"]) for g in games] == [(g["seed"], g["seat"]) for g in source]
        assert all(g["turns"] == 719 for g in games)
        def delta(key):
            return mean(a[key] - b[key] for a, b in zip(games, source))
        def profile_delta(key):
            return mean(a["profile"][key] - b["profile"][key] for a, b in zip(games, source))
        margin_delta = [(a["cash"] - a["opponent_cash"]) - (b["cash"] - b["opponent_cash"])
                        for a, b in zip(games, source)]
        record = {"name": path.stem, "games": len(games), "wins": sum(g["cash"] > g["opponent_cash"] for g in games),
                  "changed_actions": sum(a["action_hash"] != b["action_hash"] for a, b in zip(games, source)),
                  "cash_delta": delta("cash"), "rival_cash_delta": delta("opponent_cash"),
                  "margin_delta": mean(margin_delta), "margin_better": sum(d > 0 for d in margin_delta),
                  "margin_worse": sum(d < 0 for d in margin_delta), "faults_delta": delta("unit_faults"),
                  "hires_delta": profile_delta("hires"), "labor_cost_delta": profile_delta("hire_cost")}
        for key in ("produced", "sold", "discarded"):
            record[key + "_delta"] = [mean(a[key][i] - b[key][i] for a, b in zip(games, source)) for i in range(9)]
        records.append(record)
    records.sort(key=lambda r: r["margin_delta"], reverse=True)
    report = {"run": run.name, "parent_wins": sum(g["cash"] > g["opponent_cash"] for g in source),
              "scenario": parent["scenario"], "scope": "Discovery,32 common seeds and both seats against public_router. No promotion.",
              "candidates": records}
    (run / "SCREEN.json").write_text(json.dumps(report, indent=2) + "\n")
    scalar = [k for k in records[0] if not isinstance(records[0][k], list)] if records else []
    with (run / "screen.csv").open("w") as output:
        writer = csv.DictWriter(output, fieldnames=scalar)
        writer.writeheader()
        writer.writerows({k: r[k] for k in scalar} for r in records)
    for row in records[:12]:
        print(row["name"], "margin", round(row["margin_delta"], 2), "cash", round(row["cash_delta"], 2),
              "labor", round(row["labor_cost_delta"], 2), "output", row["produced_delta"])


if __name__ == "__main__":
    main()
