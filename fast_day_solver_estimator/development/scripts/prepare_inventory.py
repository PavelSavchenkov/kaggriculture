"""Index distinct development inputs and unverified physical witnesses."""
import json
from collections import Counter
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/inventory_v0"
    output.mkdir(exist_ok=False)
    records = [json.loads(line) for source in ["archive", "root_solver"]
               for line in (EXP / f"data/development/{source}/index.jsonl").read_text().splitlines()]
    problems, witnesses = {}, {}
    for row in records:
        key = row["source_sha256"]
        if key not in problems:
            problems[key] = {**row, "sources": [], "witnesses": []}
        problems[key]["sources"].append(row["source"])
        for witness in row["witnesses"]:
            pair = (key, witness["sha256"])
            if pair not in witnesses:
                witnesses[pair] = (row, witness)
                problems[key]["witnesses"].append(witness)
    (output / "features.txt").write_text("".join(
        f"{sha[:20]} {EXP / row['problem']}\n" for sha, row in sorted(problems.items())))
    (output / "audit.txt").write_text("".join(
        f"{p[:20]}_{w[:20]} {EXP / row['problem']} {EXP / witness['path']}\n"
        for (p, w), (row, witness) in sorted(witnesses.items())))
    (output / "problems.jsonl").write_text("".join(json.dumps(row, sort_keys=True) + "\n" for row in problems.values()))
    summary = {"problems": len(problems), "physical_contracts": len({r["physical_key"] for r in problems.values()}),
               "witness_pairs": len(witnesses), "records": len(records),
               "source_run_counts": dict(Counter(row["source"].split("/")[1] if row["source"].startswith("runs/")
                                                else "root_or_older" for row in records))}
    (output / "SUMMARY.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({k: v for k, v in summary.items() if k != "source_run_counts"}))


if __name__ == "__main__":
    main()
