"""Check the public CLI, preserved metrics and an independent full-course replay."""
import argparse
import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path

PACKAGE = Path(__file__).resolve().parents[1]


def execute(arguments):
    command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture",
               str(PACKAGE.parent / "day_solver/with_runtime.sh"), *map(str, arguments)]
    return subprocess.check_output(command, text=True, cwd=PACKAGE)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", type=Path, default=PACKAGE / "build")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--skip-course", action="store_true")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    cli = args.build.resolve() / "estimate_day"
    calls = [
        [cli, PACKAGE / "examples/ordinary.json"],
        [cli, PACKAGE / "examples/terminal.json", 23],
        [cli, PACKAGE / "examples/ordinary.json", 24, PACKAGE / "examples/no_hires.menu"],
    ]
    forecasts = [json.loads(execute(call)) for call in calls]
    assert forecasts[0]["uses_direct_cost"] and not forecasts[0]["certificate"]
    assert forecasts[1]["active_hours"] == 23 and not forecasts[1]["uses_direct_cost"]
    assert forecasts[2]["analytically_rejected"] and forecasts[2]["estimated_hire_cost"] is None
    cold = json.loads((PACKAGE / "evidence/reports/holdout_a_gate_v2/GATE.json").read_text())
    warm = json.loads((PACKAGE / "evidence/reports/warm_seed_benchmark_v2/SEED_REPORT.json").read_text())
    assert cold["primary_passes"] and all(cold["primary_gates"].values())
    comparison = cold["comparison"]
    assert abs(100 * (1 - comparison["after_mean_seconds"] / comparison["before_mean_seconds"]) - comparison["relative_time_reduction_percent"]) < 1e-10
    assert warm["paired_runs"] == 56 and warm["passes_strict_gate"] and all(warm["gates"].values())
    assert warm["lost_certificates"] == 0 and warm["gained_certificates"] == 4
    assert abs(1 - warm["guided_mean_cpu"] / warm["original_mean_cpu"] - warm["cpu_saving_fraction"]) < 1e-12
    course = None
    if not args.skip_course:
        output = args.output.resolve() / "COURSE.json"
        execute([args.build.resolve() / "verify_warm_course", PACKAGE / "examples/verified_course", 1470762556, output])
        course = json.loads(output.read_text())
        assert course == json.loads((PACKAGE / "examples/verified_course_expected.json").read_text())
        assert course["real_transitions"] == 719
    result = {"status": "passed", "utc": datetime.now(timezone.utc).isoformat(), "cli_cases": forecasts,
              "cold_gate_arithmetic": True, "warm_gate_arithmetic": True,
              "independent_719_transition_course": course is not None,
              "scope": "Package functionality, frozen report arithmetic, and optional existing full-course replay. No new model promotion or performance benchmark."}
    (args.output / "CHECK.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
