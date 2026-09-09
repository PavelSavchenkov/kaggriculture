"""Compare bounded-search references with independently normalized witnesses."""
import argparse
import json
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def complete_rows(path):
    return [json.loads(line) for line in path.read_text().splitlines(keepends=True) if line.endswith("\n")]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference")
    parser.add_argument("index", type=Path)
    parser.add_argument("normalized")
    parser.add_argument("output")
    args = parser.parse_args()
    output = EXP / "runs" / args.output
    output.mkdir(exist_ok=False)
    inputs = complete_rows(args.index)
    key_by_sha = {r["source_sha256"][:20]: r["physical_key"] for r in inputs}
    sources = {}
    for r in complete_rows(EXP / "runs" / args.normalized / "results.jsonl"):
        if r["status"] != "FEASIBLE":
            continue
        key = key_by_sha[r["id"].split("_")[0]]
        if key not in sources or r["workers"] < sources[key]["workers"]:
            sources[key] = r
    root = EXP / "runs" / args.reference
    outcomes = {r["id"]: r for path in (root / "initial_3s").glob("worker*/results.jsonl") for r in complete_rows(path)}
    records, audit = [], []
    for case in json.loads((root / "CASES.json").read_text()):
        ids = [f"{case['id']}_w{q['workers']:02}" for q in case["queries"]]
        if not all(key in outcomes for key in ids):
            continue
        solved = [outcomes[key]["workers"] for key in ids if outcomes[key]["status"] == "FEASIBLE"]
        source = sources.get(case["physical_key"])
        row = {"id": case["id"], "short_sweep_workers": min(solved) if solved else None,
               "normalized_source_workers": source["workers"] if source else None,
               "lower_bound": source["lower_bound"] if source else case["task_only_lower_bound"]}
        if source:
            query = next((q for q in case["queries"] if q["workers"] == source["workers"]), None)
            if query:
                audit.append(f"{case['id']} {EXP / query['path']} {EXP / 'runs' / args.normalized / (source['id'] + '.actions.txt')}\n")
        values = solved + ([source["workers"]] if source else [])
        row["merged_upper_workers"] = min(values) if values else None
        records.append(row)
    (output / "CASES.json").write_text(json.dumps(records, indent=2) + "\n")
    (output / "audit.txt").write_text("".join(audit))
    both = [r for r in records if r["normalized_source_workers"] is not None and r["short_sweep_workers"] is not None]
    missed = [r for r in both if r["normalized_source_workers"] < r["short_sweep_workers"]]
    exact = [r for r in records if r["merged_upper_workers"] == r["lower_bound"]]
    report = {"completed_cases": len(records), "both_certificates": len(both), "source_strictly_better": len(missed),
              "largest_missed_workers": max([r["short_sweep_workers"] - r["normalized_source_workers"] for r in missed], default=0),
              "lower_equals_upper": len(exact), "source_only_upper": sum(r["short_sweep_workers"] is None and r["normalized_source_workers"] is not None for r in records),
              "no_upper": sum(r["merged_upper_workers"] is None for r in records), "cross_audit_pairs": len(audit)}
    (output / "SUMMARY.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report))


if __name__ == "__main__":
    main()
