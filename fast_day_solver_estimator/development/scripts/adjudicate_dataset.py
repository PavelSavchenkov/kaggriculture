"""Tighten physical upper bounds while retaining original compiler outcomes."""
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dataset", type=Path)
    parser.add_argument("run")
    parser.add_argument("output", type=Path)
    parser.add_argument("--round", default="deeper_30s")
    args = parser.parse_args()
    root = EXP / "runs" / args.run
    status = json.loads((root / args.round / "STATUS.json").read_text())
    assert status["status"] == "completed", "freeze a complete adjudication pass"
    data = json.loads(args.dataset.read_text())
    cases = {r["id"]: r for r in data["rows"]}
    manifest = json.loads((root / "CASES.json").read_text())
    expected = {f"{r['id']}_w{q['workers']:02}" for r in manifest for q in r["queries"]}
    seen, changed = set(), []
    before = {key: row["reference_workers"] for key, row in cases.items()}
    for path in sorted((root / args.round).glob("worker*/results.jsonl")):
        for line in path.read_text().splitlines():
            result = json.loads(line); query = result["id"]
            assert query not in seen and query in expected
            seen.add(query); case = cases[query.rsplit("_w", 1)[0]]
            if result["status"] == "FEASIBLE":
                assert result["workers"] >= case["lower_bound"] and not case["deadline_missing_quantity"]
                witness = path.parent / (query + ".actions.txt")
                result["certificate"] = str(witness.relative_to(EXP))
                result["certificate_sha256"] = hashlib.sha256(witness.read_bytes()).hexdigest()
                if case["reference_workers"] is None or result["workers"] < case["reference_workers"]:
                    case["reference_workers"] = result["workers"]
            else:
                assert result["status"] == "UNKNOWN"
            case.setdefault("adjudication_outcomes", []).append(result)
    assert seen == expected
    for key, row in cases.items():
        if row["reference_workers"] != before[key]:
            changed.append({"id": key, "before": before[key], "after": row["reference_workers"],
                            "pool": row.get("pool"), "variant": row.get("variant"), "families": row["families"]})
    data.setdefault("adjudication_rounds", []).append({"run": args.run, "round": args.round, "status": status,
        "utc": datetime.now(timezone.utc).isoformat(), "dataset_sha256": hashlib.sha256(args.dataset.read_bytes()).hexdigest(),
        "manifest_sha256": hashlib.sha256((root / "CASES.json").read_bytes()).hexdigest()})
    args.output.parent.mkdir(parents=True, exist_ok=False)
    args.output.write_text(json.dumps(data, indent=2) + "\n")
    report = {"queries": len(seen), "changed": len(changed), "new_upper": sum(r["before"] is None for r in changed),
              "improved_upper": sum(r["before"] is not None for r in changed), "changes": changed,
              "interpretation": "Reference-label improvements only. Original short_sweep_workers and query_outcomes are unchanged."}
    args.output.with_name("CHANGES.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "changes"}))


if __name__ == "__main__":
    main()
