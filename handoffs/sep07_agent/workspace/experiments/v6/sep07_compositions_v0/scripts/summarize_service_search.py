"""Summarize estimates and exact realization; this script does not select policy."""
import json
from pathlib import Path

import pandas as pd

EXP = Path(__file__).resolve().parents[1]


def main():
    parent = json.loads((EXP / "results/teacher55_profile/program_55_teacher.json").read_text())
    lookup = {(g["seed"], g["seat"]): g for g in parent["games"]}
    report = {}
    for name in ("improve_care_001", "improve_care_p78_001", "improve_care_p90_001", "improve_care_p100_001", "remove_care_001", "reduce_hires_001"):
        folder = EXP / "runs" / name
        estimates = pd.read_csv(folder / "estimates.csv")
        compiled = pd.read_csv(folder / "compiled.csv")
        summary = {"estimates": len(estimates), "zero_marginal_output": int((estimates.extra_output == 0).sum()),
                   "compile_attempts": len(compiled), "solved": int(compiled.solved.sum()),
                   "matching_endpoints": int((compiled.endpoint_equal * compiled.cash_equal).sum()), "exact": {}}
        for path in sorted((folder / "exact").glob("*.json")):
            data = json.loads(path.read_text())
            gains = []
            equal = 0
            for game in data["games"]:
                old = lookup[(game["seed"], game["seat"])]
                gains.append(game["cash"] - old["cash"])
                equal += game["produced"] == old["produced"]
            summary["exact"][path.stem] = {"games": len(gains), "mean_cash_gain": sum(gains) / len(gains),
                "minimum_cash_gain": min(gains), "maximum_cash_gain": max(gains), "equal_production_games": equal}
        report[name] = summary
    (EXP / "results/service_search_summary.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
