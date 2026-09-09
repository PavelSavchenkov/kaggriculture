"""Freeze complete reference cases, certificates, bounds and physical features."""
import argparse
import csv
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def read_rows(path):
    return [json.loads(line) for line in path.read_text().splitlines(keepends=True) if line.endswith("\n")]


def collect(reference, index_path, features_path, bounds_path, normalized=None, original_path=None, reference_case_superset=False):
    root = EXP / "runs" / reference
    index = read_rows(index_path)
    by_sha = {r["source_sha256"][:20]: r for r in index}
    features = {r["id"]: r for r in csv.DictReader(features_path.open())}
    bounds = {r["id"]: r for r in csv.DictReader(bounds_path.open())}
    columns = [k for k in next(iter(features.values())) if k not in ["id", "extraction_us"]]
    original = {r["id"]: r for r in csv.DictReader(original_path.open())} if original_path else {}
    by_physical = {r["physical_key"]: r for r in index}
    outcomes = {r["id"]: r for path in (root / "initial_3s").glob("worker*/results.jsonl") for r in read_rows(path)}
    sources = {}
    if normalized:
        for row in read_rows(EXP / "runs" / normalized / "results.jsonl"):
            if row["status"] != "FEASIBLE":
                continue
            key = by_sha[row["id"].split("_")[0]]["physical_key"]
            if key not in sources or row["workers"] < sources[key]["workers"]:
                sources[key] = row
    selected, pending = [], []
    for case in json.loads((root / "CASES.json").read_text()):
        if reference_case_superset and case["physical_key"] not in by_physical:
            continue
        ids = [f"{case['id']}_w{q['workers']:02}" for q in case["queries"]]
        if not all(key in outcomes for key in ids):
            pending.append(case["id"])
            continue
        queries = [outcomes[key] for key in ids]
        successful = [q["workers"] for q in queries if q["status"] == "FEASIBLE"]
        source = sources.get(case["physical_key"])
        upper = successful + ([source["workers"]] if source else [])
        example = by_physical[case["physical_key"]]
        sha = example["source_sha256"][:20]
        bound = int(bounds[sha]["lower_bound"])
        missing = float(bounds[sha]["deadline_missing_quantity"])
        input_missing = float(bounds[sha].get("input_missing_quantity", 0))
        if upper and (bound > min(upper) or missing > 0 or input_missing > 0):
            raise ValueError("physical bound contradicts a verified schedule: " + case["id"])
        # The frozen first historical manifest predates explicit family fields.
        families = case.get("source_families", ["historical"])
        selected.append({"id": case["id"], "physical_key": case["physical_key"], "families": families, "active_hours": case.get("active_hours", 24),
                         "sources": case["sources"], "pool": example.get("pool"), "variant": example.get("variant"),
                         "reference_workers": min(upper) if upper else None,
                         "short_sweep_workers": min(successful) if successful else None,
                         "source_workers": source["workers"] if source else None,
                         "source_certificate": str(Path("runs") / normalized / (source["id"] + ".actions.txt")) if source else None,
                         "lower_bound": bound, "deadline_missing_quantity": missing,
                         "input_missing_quantity": input_missing,
                         "features": [float(features[sha][k]) for k in columns],
                         "nominal_budget_seconds": sum(q["budget_seconds"] for q in queries),
                         "actual_cpu_seconds": sum(q["cpu_seconds"] for q in queries),
                         "query_outcomes": queries,
                         "original_geometry_workers": int(original[sha]["workers"]) if original else None,
                         "original_geometry_cost": int(original[sha]["hire_cost"]) if original else None})
    return {"feature_names": columns, "rows": selected, "pending_cases": pending}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("reference")
    parser.add_argument("index", type=Path)
    parser.add_argument("features", type=Path)
    parser.add_argument("bounds", type=Path)
    parser.add_argument("name")
    parser.add_argument("--normalized")
    parser.add_argument("--original", type=Path)
    parser.add_argument("--reference-case-superset", action="store_true")
    args = parser.parse_args()
    args.index, args.features, args.bounds = args.index.resolve(), args.features.resolve(), args.bounds.resolve()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    result = collect(args.reference, args.index, args.features, args.bounds, args.normalized, args.original, args.reference_case_superset)
    result.update(utc=datetime.now(timezone.utc).isoformat(), reference=args.reference,
                  target="Cheapest verified upper bound under the frozen hiring menu, not necessarily the minimum.",
                  input_hashes={str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
                                for p in [args.index, args.features, args.bounds] + ([args.original.resolve()] if args.original else [])})
    (output / "DATASET.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"completed": len(result["rows"]), "pending": len(result["pending_cases"]),
                      "verified_upper": sum(r["reference_workers"] is not None for r in result["rows"]),
                      "proven_unreachable": sum(r["deadline_missing_quantity"] > 0 for r in result["rows"])}))


if __name__ == "__main__":
    main()
