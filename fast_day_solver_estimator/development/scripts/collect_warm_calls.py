"""Collect complete available warm-call catalogs on the exposed course panel."""
import argparse
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    output = EXP / "runs" / args.name; output.mkdir(exist_ok=False)
    specs = sorted((EXP / "data/warm_courses_v1/specs").glob("*.txt")); assert len(specs) == 7
    paths = [Path(__file__), EXP / "source/warm_compile_catalog.cpp", EXP / "scripts/run_warm_catalog.py",
             EXP / "build/warm_compile_catalog", EXP / "build/verify_warm_course", *specs]
    protocol = {"utc": datetime.now(timezone.utc).isoformat(), "status": "running", "jobs": args.jobs,
        "scope": "Exposed fixed-calendar warm compiler reference catalogs; no speed or unseen-agent claim.",
        "selection": "All seven original imported course specifications, original seed 1201301738, fixed shops/source/rival, no resumed prefix. On each reached day evaluate every constructible extra-hire count 0..6 within the forty-worker API limit.",
        "backend": "Original visit reuse at zero extras, then unchanged four-thread one-second fixed repair, then unchanged three-second cold fallback. No estimator or necessary screen changes call execution.",
        "state": "Each query starts from the same dawn and source/rival state. Restore after each hypothetical query; advance the course with its first full-game-endpoint-certified query. Stop when a day has no certificate.",
        "timing": "Per-call process CPU includes construction, artifact saving, reuse, repair, cold fallback and live-opponent endpoint checks. Component CPU and wall time are logged separately. Whole catalog time includes additional reference calls and is not an iteration-speed metric.",
        "validation": "Strict physical replay for every returned schedule and full-game endpoint checks for every executable query. Independently replay all 719 transitions of each completed selected course.",
        "sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")

    def run(spec):
        name = str((output / spec.stem).relative_to(EXP / "runs"))
        command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", "python", str(EXP / "scripts/run_warm_catalog.py"), name, str(spec)]
        with (output / (spec.stem + ".log")).open("w") as log:
            subprocess.run(command, cwd=EXP, stdout=log, stderr=subprocess.STDOUT, check=True)
        root = output / spec.stem; result = json.loads((root / "RUN.json").read_text())
        if result["returncode"] == 0:
            target = root / "INDEPENDENT.json"
            subprocess.run(["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(EXP.parents[2] / "day_solver/with_runtime.sh"),
                            str(EXP / "build/verify_warm_course"), str(root / "course"), "1201301738", str(target)], cwd=EXP, check=True)
            independent = json.loads(target.read_text()); reported = json.loads((root / "course/MATCHED_RESULT.json").read_text())
            assert all(independent[k] == reported[k] for k in ["cash", "rival_cash", "produced0", "produced1"])
            result["independent"] = independent
        print(spec.stem, "returncode", result["returncode"], "catalog_cpu", result["cpu_seconds"], flush=True)
        return {"course": spec.stem, **result}

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = list(pool.map(run, specs))
    (output / "RESULTS.json").write_text(json.dumps(results, indent=2) + "\n")
    protocol.update(status="completed", completed_utc=datetime.now(timezone.utc).isoformat())
    (output / "PROTOCOL.json").write_text(json.dumps(protocol, indent=2) + "\n")


if __name__ == "__main__":
    main()
