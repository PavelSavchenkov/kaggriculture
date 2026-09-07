"""Compare recorded C++ estimates with exact outcomes; no policy/search logic."""
import argparse
import csv
import json
from pathlib import Path
from statistics import mean
import pandas as pd


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("runs", type=Path, nargs="+")
    args = parser.parse_args()
    for run in args.runs:
        source = json.loads((run / "source.json").read_text())
        rows = []
        for record in csv.DictReader((run / "estimates.csv").open()):
            path = run / f"ticket_{record['ticket']}_to_{record['replacement']}_changed.json"
            if not path.exists():
                continue
            changed = json.loads(path.read_text())
            control = json.loads(path.with_name(path.name.replace("_changed", "_output_control")).read_text())
            row = {"ticket": int(record["ticket"]), "replacement": int(record["replacement"]),
                   "shadow": float(record["fixed_quote_cash_delta"]),
                   "market_cash": float(record["estimated_cash_delta"]),
                   "market_margin": float(record["estimated_margin_delta"])}
            for label, accept in [("seed0", lambda g: g["seed"] == 1000 and g["seat"] == 0),
                                  ("modeled_seeds", lambda g: g["seed"] < 1008),
                                  ("later_seeds", lambda g: g["seed"] >= 1008),
                                  ("all", lambda g: True)]:
                paired = [(a, b) for a, b in zip(changed["games"], source["games"]) if accept(a)]
                assert all((a["seed"], a["seat"]) == (b["seed"], b["seat"]) for a, b in paired)
                if not paired:
                    continue
                row[f"cash_{label}"] = mean(a["cash"] - b["cash"] for a, b in paired)
                row[f"margin_{label}"] = mean(a["cash"] - a["opponent_cash"] - b["cash"] + b["opponent_cash"] for a, b in paired)
            row["paired_output_control_margin"] = mean(a["cash"] - a["opponent_cash"] - b["cash"] + b["opponent_cash"] for a, b in zip(changed["games"], control["games"]))
            rows.append(row)
        frame = pd.DataFrame(rows)
        actual = [c for c in frame if c.startswith("cash_") or c.startswith("margin_")]
        correlations = frame.corr(method="spearman").loc[["shadow", "market_cash", "market_margin"], actual].round(4).to_dict()
        report = {"scope": "Selected development alternatives only. Later seeds were excluded from the16baseline traces, but are not a globally unused promotion pool. Different runs may select different alternatives.", "rows": rows, "spearman": correlations}
        (run / "calibration.json").write_text(json.dumps(report, indent=2) + "\n")
        print(run.name, "cases", len(rows), "margin correlations", correlations["margin_all"], "later", correlations.get("margin_later_seeds"))


if __name__ == "__main__":
    main()
