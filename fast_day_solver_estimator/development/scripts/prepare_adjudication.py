"""Freeze longer offline searches immediately below known workforce bounds."""
import argparse
import hashlib
import json
from pathlib import Path

from screening import screen_reason


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("name")
    parser.add_argument("--steps", type=int, default=2)
    args = parser.parse_args()
    data = json.loads(args.dataset.read_text())
    parent = EXP / "runs" / data["reference"]
    cases = {c["id"]: c for c in json.loads((parent / "CASES.json").read_text())}
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    selected = []
    for row in data["rows"]:
        if screen_reason(row):
            continue
        case = cases[row["id"]]
        upper = row["reference_workers"]
        if upper is None:
            queries = case["queries"][-args.steps:]
        else:
            queries = [q for q in case["queries"] if max(row["lower_bound"], upper - args.steps) <= q["workers"] < upper]
        if queries:
            selected.append({**case, "queries": queries, "incumbent_workers": upper,
                             "verified_lower_bound": row["lower_bound"]})
    (output / "CASES.json").write_text(json.dumps(selected, indent=2) + "\n")
    protocol = {"purpose": "Tighten reference evidence with unchanged root V30. This is label work, not estimator improvement.",
                "selection": f"For every completed non-screened case, search up to{args.steps} counts below its best verified upper bound, respecting the necessary lower bound. For no-upper cases search the top counts in the original range.",
                "source_dataset_sha256": hashlib.sha256(args.dataset.read_bytes()).hexdigest(),
                "cases": len(selected), "queries": sum(len(c["queries"]) for c in selected),
                "query_budget_seconds": 30, "threads_per_query": 1, "schedule_validation": "Independent strict replay",
                "infeasibility": "Search failure is UNKNOWN. Keep all prior certificates."}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")
    print(json.dumps({k: protocol[k] for k in ["cases", "queries", "query_budget_seconds"]}))


if __name__ == "__main__":
    main()
