"""Verify that the fixture adapter reproduces unchanged original-shop evidence."""
import hashlib
import json
import os
import subprocess
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    output = EXP / "runs/warm_shop_adapter_check_v1"; output.mkdir(exist_ok=False)
    environment = dict(os.environ); environment["LABOR_WARM_SHOPS"] = "52555401"
    runtime = EXP.parents[2] / "day_solver/with_runtime.sh"
    seed = 1201301738
    command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(runtime), str(EXP / "build/warm_shop_season"), str(output / "source"), str(seed)]
    with (output / "source.log").open("w") as stream:
        subprocess.run(command, cwd=EXP, env=environment, stdout=stream, stderr=subprocess.STDOUT, check=True)
    original = EXP / "runs/warm_source_v1"
    before = json.loads((original / "REPORT.json").read_text()); after = json.loads((output / "source/REPORT.json").read_text())
    for key in ["source_action_hash", "rival_action_hash", "source_cash", "rival_cash"]:
        assert before[key] == after[key], key
    file_checks = []
    for day in range(30):
        for name in ["problem.json", "actions.txt", "economic_actions.txt"]:
            a, b = original / str(day) / name, output / "source" / str(day) / name
            assert a.read_bytes() == b.read_bytes(), (day, name)
            file_checks.append({"day": day, "name": name, "sha256": hashlib.sha256(a.read_bytes()).hexdigest()})
    courses = [EXP / "runs/warm_stage_controls_v1/wheat12_d15", EXP / "runs/warm_seed_benchmark_v2/jobs/s2034920637_melon12_d12/original"]
    for index, course in enumerate(courses):
        case_seed = seed if index == 0 else 2034920637
        target = output / f"independent_{index}.json"
        subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(runtime), str(EXP / "build/verify_warm_shop_course"),
                        str(course / "course"), str(case_seed), str(target)], cwd=EXP, env=environment, check=True)
        expected = json.loads((course / "INDEPENDENT.json").read_text())
        assert json.loads(target.read_text()) == expected
    report = {"utc": datetime.now(timezone.utc).isoformat(), "status": "passed", "shop_sequence": environment["LABOR_WARM_SHOPS"],
              "source_summary_fields_equal": 4, "source_day_artifacts_identical": len(file_checks), "independent_course_replays_equal": len(courses),
              "real_transitions_per_course": 719, "files": file_checks,
              "input_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [Path(__file__), EXP / "include/warm_shops.hpp", EXP / "include/warm_shop_season.hpp", EXP / "build/warm_shop_season", EXP / "build/verify_warm_shop_course", EXP / "runs/warm_shop_source_v1/SOURCE.patch"]}}
    (output / "CHECK.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k, v in report.items() if k not in ["files", "input_sha256"]}))


if __name__ == "__main__":
    main()
