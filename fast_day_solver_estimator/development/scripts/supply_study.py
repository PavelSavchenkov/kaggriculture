"""Audit conservative supply bounds against source and compiler witnesses."""
import argparse
import csv
import hashlib
import json
import subprocess
from collections import defaultdict
from pathlib import Path

import numpy as np

from import_problems import physical_key


EXP = Path(__file__).resolve().parents[1]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--indices", type=Path, nargs="+", required=True)
    parser.add_argument("--references", type=Path, nargs="+", default=[])
    parser.add_argument("--datasets", type=Path, nargs="+", default=[])
    args = parser.parse_args()
    output = EXP / "runs" / args.name
    output.mkdir(exist_ok=False)
    inputs, witnesses, physical, provenance = {}, {}, {}, {}

    def add(problem, hours, key, witness=None):
        sha = digest(problem)
        identity = sha[:20], hours
        inputs[identity] = problem
        physical[identity] = key
        if witness:
            witnesses[sha[:20], digest(witness)[:20], hours] = problem, witness

    for path in args.indices:
        provenance[str(path)] = digest(path)
        for row in map(json.loads, path.read_text().splitlines()):
            p, hours = EXP / row["problem"], row.get("active_hours", 24)
            assert digest(p) == row["source_sha256"]
            assert physical_key(json.loads(p.read_text()), hours) == row["physical_key"]
            add(p, hours, row["physical_key"])
            for witness in row.get("witnesses", []):
                add(p, hours, row["physical_key"], EXP / witness["path"])
    for reference in args.references:
        manifest = reference / "CASES.json"
        provenance[str(manifest)] = digest(manifest)
        cases = json.loads(manifest.read_text())
        queries = {f"{c['id']}_w{q['workers']:02}": (c, q) for c in cases for q in c["queries"]}
        best = {}
        for status in sorted(reference.glob("*/STATUS.json")):
            assert json.loads(status.read_text())["status"] == "completed", "do not audit a moving reference round"
            for results in sorted(status.parent.glob("worker*/results.jsonl")):
                provenance[str(results)] = digest(results)
                for row in map(json.loads, results.read_text().splitlines()):
                    if row["status"] != "FEASIBLE":
                        continue
                    case, query = queries[row["id"]]
                    key = case["physical_key"]
                    if key not in best or row["workers"] < best[key][0]:
                        best[key] = row["workers"], case, query, results.parent / (row["id"] + ".actions.txt")
        for _, case, query, witness in best.values():
            add(EXP / query["path"], case.get("active_hours", 24), case["physical_key"], witness)
    rows, audit = [], []
    for hours in [24, 23]:
        if not any(h == hours for _, h in inputs):
            continue
        manifest = output / f"features_{hours}.txt"
        manifest.write_text("".join(f"{sha} {path}\n" for (sha, h), path in sorted(inputs.items()) if h == hours))
        audit_manifest = output / f"audit_{hours}.txt"
        audit_manifest.write_text("".join(f"{sha}_{wsha} {p} {w}\n" for (sha, wsha, h), (p, w) in sorted(witnesses.items()) if h == hours))
        for mode, source in [("features", manifest), ("audit", audit_manifest)]:
            target = output / f"{mode}_{hours}.csv"
            command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                       str(EXP / "build/supply_tool"), mode, str(source), str(target), str(hours)]
            subprocess.run(command, check=True, cwd=EXP)
            values = list(csv.DictReader(target.open()))
            (rows if mode == "features" else audit).extend(values)
    by_physical = {}
    names = [k for k in rows[0] if k not in ["id", "active_hours", "supply_us"]]
    for row in rows:
        key = physical[row["id"], int(row["active_hours"])]
        values = {k: float(row[k]) for k in names}
        if key in by_physical:
            assert by_physical[key] == values, "physical identity changed supply features"
        by_physical[key] = values
    summaries = []
    for path in args.datasets:
        data = json.loads(path.read_text())
        cases = []
        for row in data["rows"]:
            supply = by_physical[row["physical_key"]]
            if row["reference_workers"] is not None:
                assert supply["supply_lower_bound"] <= row["reference_workers"] and not supply["supply_missing"]
            cases.append({"id": row["id"], "family": row["families"], "reference_workers": row["reference_workers"],
                          "old_lower_bound": row["lower_bound"], "old_unreachable": row["deadline_missing_quantity"] > 0, **supply})
        summaries.append({"dataset": str(path), "cases": len(cases),
                          "new_unreachable": sum(r["supply_missing"] > 0 and not r["old_unreachable"] for r in cases),
                          "known_upper": sum(r["reference_workers"] is not None for r in cases),
                          "tightened_finite_bound": sum(r["old_lower_bound"] < r["supply_lower_bound"] <= 40 for r in cases),
                          "outside_40_capacity_without_infeasibility_proof": sum(r["supply_lower_bound"] > 40 and not r["supply_missing"] for r in cases),
                          "unresolved_after_screens": sum(r["reference_workers"] is None and not r["supply_missing"] and not r["old_unreachable"] for r in cases)})
        target = output / (path.parent.name + "_CASES.json")
        target.write_text(json.dumps(cases, indent=2) + "\n")
    times = np.array([float(r["supply_us"]) for r in rows])
    report = {"feature_inputs": len(rows), "physical_inputs": len(by_physical), "audited_witnesses": len(audit),
              "valid_witnesses": sum(int(r["strict"]) for r in audit), "witness_contradictions": 0,
              "supply_microseconds": {"mean": float(times.mean()), "p50": float(np.quantile(times, .5)), "p95": float(np.quantile(times, .95))},
              "timing_scope": "One timed call per loaded input; includes hire-menu construction, excludes JSON parsing and subsequent audit checks.",
              "datasets": summaries, "input_sha256": provenance}
    (output / "PHYSICAL_FEATURES.json").write_text(json.dumps(by_physical, indent=2) + "\n")
    (output / "REPORT.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k != "input_sha256"}, indent=2))


if __name__ == "__main__":
    main()
