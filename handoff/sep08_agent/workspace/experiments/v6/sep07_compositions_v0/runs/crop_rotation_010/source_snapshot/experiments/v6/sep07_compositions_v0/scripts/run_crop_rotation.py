"""Freeze the complete C++ inputs and run a dated crop-family compilation."""
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
    parser.add_argument("--cell", default="10", help="Comma-separated cells in one jointly compiled crop block.")
    parser.add_argument("--seconds", type=float, default=5)
    parser.add_argument("--estimate-only", action="store_true")
    parser.add_argument("--cancel-unused-seeds", action="store_true")
    parser.add_argument("--next-day-tomato-sales", action="store_true")
    parser.add_argument("--resume-day", type=int, choices=[25, 26], default=26)
    parser.add_argument("--force-berry", action="store_true", help="Compile the complete optional day20 berry continuation.")
    args = parser.parse_args()
    assert args.run.replace("_", "").isalnum()
    out = EXP / "runs" / args.run
    launch = EXP / "runs" / (args.run + "_launch")
    assert not out.exists() and not launch.exists()
    launch.mkdir()
    build = EXP / "build/day_scheduler"
    compiler_target = "compile_crop_rotation_berry" if args.force_berry else "compile_crop_rotation"
    binary = build / compiler_target
    files = {Path(__file__).resolve(), EXP / "scheduler/CMakeLists.txt"}
    for dependency in (build / f"CMakeFiles/{compiler_target}.dir").rglob("*.o.d"):
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
               str(args.seed), str(args.cell), str(args.seconds)]
    if args.estimate_only:
        command.append("--estimate-only")
    if args.cancel_unused_seeds:
        command.append("--cancel-unused-seeds")
    if args.next_day_tomato_sales:
        command.append("--next-day-tomato-sales")
    if args.resume_day != 26:
        command.append(f"--resume-day={args.resume_day}")
    record = {"started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(), "command": command,
              "source_sha256": hashes, "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
              "source_parent": "crop_value_m2_t4 with day20 berry branch forced on" if args.force_berry else "candidates/investment_context_guarded_001_best",
              "compiler_target": compiler_target,
              "force_berry": args.force_berry,
              "lineage": f"Mengfei Li episode106429645 seat0 cell(0,1), submission56047440: tomato plant13, water13/15/17/19/20/21/22/23, fertilize20/23, harvest21..24; local implementation retains mature source wheat harvest12/13 and resumes carrot from{args.resume_day}. See research/crop_conversion_audit_001 for hashes and exact donor actions. Source extractor copied from improve_fertilization.cpp. V30 replans all affected days and declares changed sales/feed supply/seed stock. Command records optional removed seed purchases and next-morning surplus sales."}
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
