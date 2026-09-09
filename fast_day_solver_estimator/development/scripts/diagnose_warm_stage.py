"""Diagnose the completed cold-stage benchmark without changing its quality gate."""
import argparse
import csv
import hashlib
import json
from pathlib import Path

from import_problems import physical_key


def rows(path):
    return list(csv.DictReader(path.open()))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("benchmark", type=Path)
    args = parser.parse_args()
    root = args.benchmark
    report = json.loads((root / "REPORT.json").read_text())
    compared, differences, deferred_successes, input_differences = [], [], [], []
    for pair in report["pairs"]:
        dirs = {m: root / "jobs" / pair["id"] / m / "course" for m in ["original", "guided"]}
        calls = {m: {(int(r["day"]), int(r["extra_hires"])): r for r in rows(d / "compile.csv")} for m, d in dirs.items()}
        guidance = rows(dirs["guided"] / "guidance.csv")
        for key in sorted(calls["original"].keys() & calls["guided"].keys()):
            day, extra = key
            paths = {m: d / "days" / str(day) / f"problem_h{extra}.json" for m, d in dirs.items()}
            values = {m: json.loads(p.read_text()) for m, p in paths.items()}
            a, b = calls["original"][key], calls["guided"][key]
            item = {"pair": pair["id"], "day": day, "extra": extra,
                    "same_full_problem": values["original"] == values["guided"],
                    "same_physical_problem": physical_key(values["original"], 23 if day == 29 else 24) == physical_key(values["guided"], 23 if day == 29 else 24),
                    "original": a, "guided": b,
                    "problem_sha256": {m: hashlib.sha256(p.read_bytes()).hexdigest() for m, p in paths.items()}}
            compared.append(item)
            if not item["same_full_problem"]: input_differences.append(item)
            if (a["solved"], a["endpoint"]) != (b["solved"], b["endpoint"]): differences.append(item)
        for row in guidance:
            key = int(row["day"]), int(row["extra_hires"])
            if row["deferred"] == "1" and key in calls["original"] and calls["original"][key]["endpoint"] == "1":
                deferred_successes.append({"pair": pair["id"], **row})
    result = {"scope": "Post hoc diagnosis of completed cold-stage seed pairs; not a replacement benchmark or a change to its quality gate.",
              "common_executed_calls": len(compared), "same_full_problem_calls": sum(r["same_full_problem"] for r in compared),
              "different_execution_outcomes": differences, "deferred_original_successes": deferred_successes,
              "different_full_inputs": input_differences,
              "interpretation": "A stage deferral follows repair failure in that candidate run. A successful paired original call may instead have succeeded in repair; these logs do not identify that original stage. Differences on identical full inputs can also reflect finite-budget solver or full-game endpoint variation. The completed benchmark's strict quality gate is unchanged by this diagnosis.",
              "input_sha256": {"REPORT.json": hashlib.sha256((root / "REPORT.json").read_bytes()).hexdigest(),
                               "script": hashlib.sha256(Path(__file__).read_bytes()).hexdigest()}}
    with (root / "DIAGNOSIS.json").open("x") as stream:
        stream.write(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k not in ["different_execution_outcomes", "different_full_inputs", "input_sha256"]}, indent=2))
    print("Different execution outcomes:", [(r["pair"], r["day"], r["extra"], r["same_full_problem"]) for r in differences])


if __name__ == "__main__":
    main()
