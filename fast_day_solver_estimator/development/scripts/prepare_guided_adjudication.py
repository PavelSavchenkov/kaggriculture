"""Allocate offline reference calls across plausible workforces for unknown cases."""
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("forecasts", type=Path)
    parser.add_argument("name")
    parser.add_argument("--queries", type=int, default=3)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text()); assert not data.get("pending_cases")
    manifest = EXP / "runs" / data["reference"] / "CASES.json"
    cases = {r["id"]: r for r in json.loads(manifest.read_text())}
    forecasts = {r["query_id"]: r for r in map(json.loads, args.forecasts.read_text().splitlines()) if r["method"] == "boost"}
    selected, decisions = [], []
    for row in data["rows"]:
        if row["reference_workers"] is not None or screen_reason(row):
            continue
        tried = {q["workers"] for q in row.get("adjudication_outcomes", [])}
        case = cases[row["id"]]
        available = [q for q in case["queries"] if q["workers"] >= row["lower_bound"] and q["workers"] not in tried]
        def score(query):
            key = f"{row['id']}_w{query['workers']:02}"
            return forecasts[key]["predicted_success"]
        ordered = sorted(available, key=lambda q: (-score(q), q["workers"]))
        chosen = []
        for separation in [3, 1]:
            for query in ordered:
                if len(chosen) == args.queries:
                    break
                if all(abs(query["workers"] - other["workers"]) >= separation for other in chosen):
                    chosen.append(query)
        if chosen:
            selected.append({**case, "queries": chosen, "incumbent_workers": None, "verified_lower_bound": row["lower_bound"]})
            decisions.append({"id": row["id"], "family": row["families"], "lower_bound": row["lower_bound"],
                "previous_long_counts": sorted(tried), "selected": [{"workers": q["workers"], "predicted_three_second_success": score(q)} for q in chosen]})
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    (output / "CASES.json").write_text(json.dumps(selected, indent=2) + "\n")
    (output / "DECISIONS.json").write_text(json.dumps(decisions, indent=2) + "\n")
    report = {"utc": datetime.now(timezone.utc).isoformat(), "purpose": "Improve offline reference evidence for unresolved obligations. This is not an online policy benchmark or estimator improvement.",
        "selection": "Unknown non-screened cases only. Rank unused longer-budget worker counts by learned three-second success probability; greedily spread selected counts by at least three workers, then fill remaining slots. No repeated longer-budget calls.",
        "limitation": "The probability model was trained on exposed three-second observations, including these development cases. Its ranking need not transfer to thirty-second solves. Failure remains UNKNOWN; no worker-count cap is imposed beyond the API's forty.",
        "cases": len(selected), "queries": sum(len(r["queries"]) for r in selected), "query_budget_seconds": 30,
        "root_solver": "V30 unchanged", "validation": "Independent strict endpoint replay", "threads_per_query": 1,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.dataset, args.forecasts, manifest, Path(__file__)]}}
    (output / "PROTOCOL.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ["cases", "queries", "query_budget_seconds"]}))


if __name__ == "__main__":
    main()
