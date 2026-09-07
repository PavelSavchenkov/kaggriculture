"""Freeze inputs and launch the C++ crop-service compilation experiment."""
import argparse
import datetime
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run")
    parser.add_argument("--seed", type=int, default=1000)
    parser.add_argument("--limit", type=int, default=24)
    parser.add_argument("--seconds", type=float, default=5)
    args = parser.parse_args()
    assert args.run.replace("_", "").isalnum()
    out = EXP / "runs" / args.run
    launch = EXP / "runs" / (args.run + "_launch")
    assert not out.exists() and not launch.exists()
    launch.mkdir()
    build = EXP / "build/day_scheduler"
    binary = build / "improve_fertilization"
    files = {Path(__file__).resolve(), EXP / "scheduler/CMakeLists.txt"}
    for dependency in (build / "CMakeFiles/improve_fertilization.dir").rglob("*.o.d"):
        for name in dependency.read_text().replace("\\\n", " ").split()[1:]:
            path = Path(name).resolve()
            if path.is_relative_to(ROOT) and path.is_file():
                files.add(path)
    hashes = {}
    for path in sorted(files):
        relative = path.relative_to(ROOT)
        target = launch / "source_snapshot" / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
        hashes[str(relative)] = hashlib.sha256(path.read_bytes()).hexdigest()
    command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", str(binary), str(out),
               str(args.seed), str(args.limit), str(args.seconds)]
    record = {"started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "command": command,
              "source_sha256": hashes, "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
              "source_parent": "candidates/investment_context_guarded_001_best",
              "lineage": "Local dated crop-service proposals; V30 whole-day schedules. Funding, guard and export helpers copied unchanged from compile_animal_entries.cpp and rebuild_investment_days.cpp. Parent external replay lineage remains in its IMPORT.json.",
              "estimate_scope": "Source-lifecycle harvest totals must match dated biology. Rank extra production at current observed prices less one fertilizer purchase; heuristic excludes labor and market impact. Full exact games measure all later consequences."}
    (launch / "RUN.json").write_text(json.dumps(record, indent=2) + "\n")
    with (EXP / "results" / (args.run + ".log")).open("w") as log:
        result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT)
    record["returncode"] = result.returncode
    record["finished_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    (launch / "RUN.json").write_text(json.dumps(record, indent=2) + "\n")
    if out.exists():
        shutil.move(launch / "RUN.json", out / "RUN.json")
        shutil.move(launch / "source_snapshot", out / "source_snapshot")
        launch.rmdir()
    print(args.run, "returncode", result.returncode)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
