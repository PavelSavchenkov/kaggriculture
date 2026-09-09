"""Observable first-certificate stopping diagnostics for a completed C++ replay."""
import argparse
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

import numpy as np

from baseline_study import cost
from improvement_report import bootstrap


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("replay", type=Path)
    args = parser.parse_args()
    root = args.replay
    definitions = json.loads((root / "DEFINITIONS.json").read_text())
    calls = defaultdict(list)
    for row in csv.DictReader((root / "cpp/CALLS.csv").open()):
        calls[row["test"]].append(row)
    measured = [json.loads(line) for line in (root / "MEASURED_CASES.jsonl").read_text().splitlines()]
    best = {r["pool"]: r["oracle_cost"] for r in measured}
    rows = []
    for definition in definitions:
        first = next((r for r in calls[definition["test"]] if r["success"] == "1"), None)
        bill = float(cost([int(first["best_workers"])])[0]) if first else None
        rows.append({"pool": definition["pool"], "family": definition["family"], "method": definition["policy"],
            "first_certificate_cpu_seconds": float(first["elapsed_cpu_seconds"]) if first else None,
            "first_certificate_bill": bill, "reference_best_bill": best[definition["pool"]],
            "regret": bill - best[definition["pool"]] if first else None,
            "calls_until_certificate": int(first["step"]) + 1 if first else None})
    grouped = defaultdict(list)
    for row in rows: grouped[row["method"]].append(row)
    summary = []
    for method, group in sorted(grouped.items()):
        known = [r for r in group if r["first_certificate_bill"] is not None]
        times = [r["first_certificate_cpu_seconds"] for r in known]
        summary.append({"method": method, "pools": len(group), "certified_pools": len(known),
            "mean_time_to_first_certificate": float(np.mean(times)) if times else None,
            "p95_time_to_first_certificate": float(np.quantile(times, .95)) if times else None,
            "mean_first_bill": float(np.mean([r["first_certificate_bill"] for r in known])) if known else None,
            "mean_first_bill_regret": float(np.mean([r["regret"] for r in known])) if known else None,
            "first_certificate_is_best": sum(r["regret"] == 0 for r in known) / len(group)})
    lookup = {(r["method"], r["pool"]): r for r in rows}
    comparisons = []
    after = "cost_direct_extra/around"
    for before in ["original_geometry/around", "original_geometry/sequential", "original_flat_10/sequential"]:
        pairs = []
        for candidate in grouped[after]:
            baseline = lookup[before, candidate["pool"]]
            if baseline["first_certificate_bill"] is None or candidate["first_certificate_bill"] is None:
                continue
            pairs.append({"family": candidate["family"], "pool": candidate["pool"],
                "seconds_saved": baseline["first_certificate_cpu_seconds"] - candidate["first_certificate_cpu_seconds"],
                "bill_saved": baseline["first_certificate_bill"] - candidate["first_certificate_bill"]})
        comparisons.append({"before": before, "after": after, "common_pools": len(pairs),
            "mean_seconds_saved": float(np.mean([r["seconds_saved"] for r in pairs])),
            "saved_seconds_family_95": bootstrap(pairs, lambda sample: float(np.mean([r["seconds_saved"] for r in sample]))),
            "mean_bill_saved": float(np.mean([r["bill_saved"] for r in pairs])),
            "saved_bill_family_95": bootstrap(pairs, lambda sample: float(np.mean([r["bill_saved"] for r in sample])))})
    result = {"scope": "Post hoc diagnostic of an observable stop-after-first-certificate rule. Not an additional predeclared acceptance gate or an original warm-compiler benchmark.",
        "timing": "Measured C++ scoring and decisions plus recorded compiler CPU through the first strict success. No reference-best label enters this stopping rule.",
        "summary": summary, "comparisons": comparisons, "pairs": rows,
        "input_sha256": {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in [root / "DEFINITIONS.json", root / "cpp/CALLS.csv", root / "MEASURED_CASES.jsonl", Path(__file__)]}}
    with (root / "FIRST_CERTIFICATE.json").open("x") as stream: stream.write(json.dumps(result, indent=2) + "\n")
    print(json.dumps({"summary": summary, "comparisons": comparisons}, indent=2))


if __name__ == "__main__":
    main()
