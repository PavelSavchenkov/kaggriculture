"""Check the package using bundled exposed cases. No downloads or reserved data."""
import argparse
import hashlib
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

from build import EXP, build
from prepare_data import prepare


def run(command, directory, name):
    result = subprocess.run(list(map(str, command)), capture_output=True, text=True)
    (directory / f"{name}.stdout").write_text(result.stdout)
    (directory / f"{name}.stderr").write_text(result.stderr)
    result.check_returncode()
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--live", action="store_true", help="Also build generic and typed debug agent runners")
    parser.add_argument("--output", type=Path, default=EXP / "runs/package_check")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    paths = [r["calendar"] for r in prepare([107273702, 107323879, 107342138], archive=False)]
    report = {"checked_utc": datetime.now(timezone.utc).isoformat(), "episodes": 3, "checks": {}}
    for name in ["check_market", "check_continuation_resume", "check_purchase_witness", "check_rival_delivery"]:
        print(f"Building and running {name}", flush=True)
        binary = build(EXP / f"source/{name}.cpp")
        arguments = ["16"] if name == "check_market" else paths
        output = run([binary, *arguments], args.output, name)
        report["checks"][name] = {"binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                                  "result": [json.loads(line) for line in output.splitlines()]}
    print("Comparing replay results with the frozen session binary", flush=True)
    binary = build(EXP / "source/replay_timing.cpp")
    output = run([binary, "--source-actions", "--delivery-bound", "--purchase-witness", "--multiple-sales", *paths],
                 args.output, "replay")
    rows = [json.loads(line) for line in output.splitlines()]
    for row in rows:
        row.pop("seconds")
    expected = json.loads((EXP / "evidence/FIXTURE_EXPECTED.json").read_text())["rows"]
    # Later experimental diagnostics were added after the frozen timing build.
    # They must stay zero for this retained policy; compare every older field.
    added = {"holding_exchanges", "prebought_seeds", "sale_advances", "advanced_units", "seed_prebuys"}
    for row in rows:
        for key in added:
            if row.pop(key) != 0:
                raise AssertionError(f"An unpromoted experiment is active: {key}")
    if rows != expected:
        raise AssertionError("Packaged replay results differ from the frozen session; inspect replay.stdout")
    report["checks"]["frozen_replay"] = {"exact_rows": len(rows), "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest()}
    if args.live:
        def arena(name, flags):
            print(f"Building {name}", flush=True)
            output = run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python",
                          EXP / "scripts/build_arena.py", *flags], args.output, name)
            return Path(output.strip().splitlines()[-1])
        generic = arena("generic_build", ["--agents", "history_course_multiple", "room_keep"])
        debug = arena("pair_debug_build", ["--pair", "history_course_multiple", "pass", "--debug"])
        def match(binary, name, a, b, threads=1, budget=100000):
            target = args.output / f"{name}.json"
            run([binary, "--validate", "--a", a, "--b", b, "--games", "2", "--threads", str(threads),
                 "--seed-start", "2026091061000", "--budget-expansions", str(budget), "--output", target],
                args.output, name)
            result = json.loads(target.read_text())
            if not result["validated"] or any(g["turns"] != 719 for g in result["games"]):
                raise AssertionError(f"Action or terminal check failed: {name}")
            return result["games"]
        serial = match(generic, "pass_generic", "history_course_multiple", "pass")
        parallel = match(debug, "pass_debug", "history_course_multiple", "pass", threads=2)
        if serial != parallel:
            raise AssertionError("Generic and parallel typed-debug games differ")
        frozen = json.loads((EXP / "evidence/FIXTURE_EXPECTED.json").read_text())["live_reference"]["games"]
        if serial != frozen:
            raise AssertionError("Live results differ from the frozen validated session binary")
        selfplay = match(generic, "selfplay", "history_course_multiple", "history_course_multiple", threads=2)
        zero = match(generic, "zero_budget", "history_course_multiple", "pass", budget=0)
        room = match(generic, "room_keep", "room_keep", "pass")
        report["checks"]["live"] = {"exact_generic_pair_games": len(serial), "selfplay_games": len(selfplay),
                                    "zero_budget_games": len(zero), "room_keep_games": len(room),
                                    "exact_frozen_games": len(serial),
                                    "baseline_unit_faults": [g["unit_faults"] for g in serial]}
    (args.output / "SUMMARY.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
