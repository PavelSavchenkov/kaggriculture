"""Collect explicit-calendar reference evidence without merging menu variants."""
import argparse
import csv
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("data", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--references", type=Path, nargs="+", required=True)
    parser.add_argument("--round", default="initial_3s")
    parser.add_argument("--terminal-round")
    args = parser.parse_args()
    paths = [args.data / "index.jsonl"]
    features = {}
    active = sorted({case["active_hours"] for reference in args.references for case in json.loads((reference / "CASES.json").read_text())})
    columns = None
    for hours in active:
        path = args.data / f"FEATURES_{hours}.csv"; paths.append(path)
        rows = list(csv.DictReader(path.open()))
        names = [k for k in rows[0] if k not in ["id", "extraction_us"]]
        assert columns is None or columns == names
        columns = names
        features.update((r["id"], r) for r in rows)
    records = []
    for reference in args.references:
        manifest = reference / "CASES.json"
        cases = json.loads(manifest.read_text())
        assert len({case["active_hours"] for case in cases}) == 1
        round_name = args.terminal_round if cases[0]["active_hours"] == 23 and args.terminal_round else args.round
        status = reference / round_name / "STATUS.json"
        assert json.loads(status.read_text())["status"] == "completed", "wait for the complete reference round"
        backend = json.loads(status.read_text()); paths.extend([manifest, status])
        outcomes = {}
        for results in (reference / round_name).glob("worker*/results.jsonl"):
            paths.append(results)
            for row in map(json.loads, results.read_text().splitlines()):
                assert row["id"] not in outcomes
                outcomes[row["id"]] = row
        for case in cases:
            qs = [outcomes[f"{case['id']}_w{q['workers']:02}"] for q in case["queries"]]
            f = features[case["id"]]
            lower = int(float(f["planning_lower_bound"]))
            missing = {name: float(f[name]) for name in ["deadline_missing_quantity", "supply_missing", "seed_missing", "land_missing"]}
            solved = [q["workers"] for q in qs if q["status"] == "FEASIBLE"]
            if solved:
                assert lower <= min(solved) and not any(missing.values()), case["id"]
            records.append({"id": case["id"], "contract_key": case["contract_key"], "obligation_key": case["obligation_key"],
                            "families": case["source_families"], "active_hours": case["active_hours"], "hire_slots": case["hire_slots"],
                            "profile": case["profile"], "parent_panel": case["parent_panel"],
                            "reference_round": round_name, "reference_binary_sha256": backend["binary_sha256"],
                            "explicit_terminal_work_deadlines": backend.get("terminal_work_deadlines", False),
                            "reference_workers": min(solved) if solved else None, "short_sweep_workers": min(solved) if solved else None,
                            "lower_bound": lower, **missing, "query_outcomes": qs,
                            "features": [float(f[name]) for name in columns],
                            "actual_cpu_seconds": sum(q["cpu_seconds"] for q in qs), "nominal_budget_seconds": sum(q["budget_seconds"] for q in qs)})
    assert len({r["id"] for r in records}) == len(records) == len(features)
    result = {"utc": datetime.now(timezone.utc).isoformat(), "schema": "explicit_calendar_v1",
              "target": "Cheapest verified upper bound for this exact optional-hire menu and horizon; UNKNOWN remains censored.",
              "feature_names": columns, "rows": records, "pending_cases": [],
              "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    with args.output.open("x") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"contracts": len(records), "obligations": len({r["obligation_key"] for r in records}),
                      "verified_upper": sum(r["reference_workers"] is not None for r in records)}))


if __name__ == "__main__":
    main()
