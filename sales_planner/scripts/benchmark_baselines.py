import argparse
import json
import statistics
import subprocess
from pathlib import Path

from build import EXP, build


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--timing-only", action="store_true")
    parser.add_argument("--compact-only", action="store_true")
    parser.add_argument("--storage-only", action="store_true")
    parser.add_argument("--scenario-library", type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True)
    binary = build(EXP / "source/benchmark_baselines.cpp")
    files = sorted(args.directory.glob("*.calendar"))
    assert files
    assert sum([args.timing_only, args.compact_only, args.storage_only]) <= 1
    mode = ["--timing"] if args.timing_only else ["--compact"] if args.compact_only else ["--storage"] if args.storage_only else []
    if args.scenario_library:
        assert not mode
        specification = json.loads(args.scenario_library.read_text())
        sources = [(EXP / name).resolve() for name in specification["calendars"]]
        files = [f for f in files if f.resolve() not in sources]
        mode = ["--scenarios", str(len(sources)), *map(str, sources)]
    command = [str(binary), *mode, *map(str, files)]
    with (args.output / "results.jsonl").open("w") as out:
        subprocess.run(command, check=True, stdout=out)
    rows = [json.loads(line) for line in (args.output / "results.jsonl").read_text().splitlines()]
    original = {(r["episode"], r["seat"]): r for r in rows if r["policy"] == "original"}
    summary = []
    for policy in sorted({r["policy"] for r in rows}):
        selected = [r for r in rows if r["policy"] == policy]
        own_ok = [r for r in selected if r["own_missing"] == 0 and r["own_commitment_failures"] == 0]
        both_ok = [r for r in own_ok if r["rival_missing"] == 0 and r["rival_commitment_failures"] == 0]
        gains = [r["margin"] - original[r["episode"], r["seat"]]["margin"] for r in both_ok]
        summary.append({"policy": policy, "cases": len(selected), "own_feasible": len(own_ok),
                        "both_feasible": len(both_ok), "conditional_mean_margin_gain": statistics.mean(gains) if gains else None,
                        "mean_ms": 1000 * statistics.mean(r["seconds"] for r in selected),
                        "median_ms": 1000 * statistics.median(r["seconds"] for r in selected)})
    report = {"command": command, "summary": summary,
              "scope": "Exposed development calendars, original market worlds, fixed rival orders. Conditional gains are descriptive survivor subsets, not promotion evidence; all failures retained."}
    (args.output / "SUMMARY.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
