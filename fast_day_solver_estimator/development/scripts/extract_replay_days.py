"""Verify replay parity and extract only the selected team's development days."""
import argparse
import csv
import hashlib
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("traces", type=Path)
    args = parser.parse_args()
    traces = args.traces.resolve()
    manifest = json.loads((traces / "MANIFEST.json").read_text())
    output = EXP / "data" / traces.name.replace("_traces_v1", "_days")
    output.mkdir(exist_ok=False)
    jobs = [(game, seat) for game in manifest["games"] if game["status"] == "exported_unverified" for seat in game["seats"]]

    def run(job):
        game, seat = job
        name = f"{game['episode']}_p{seat['seat']}"
        folder = output / name
        command = ["conda", "run", "--no-capture-output", "-n", "kaggriculture",
                   str(REPO / "day_solver/with_runtime.sh"), str(EXP / "build/extract_replay"),
                   str(EXP / game["trace"]), str(seat["seat"]), str(folder)]
        with (output / f"{name}.log").open("w") as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, cwd=REPO)
        record = {"episode": game["episode"], **seat, "exit_code": result.returncode, "command": command,
                  "raw_replay_sha256": game["replay_sha256"], "days": []}
        if result.returncode == 0:
            for day in csv.DictReader((folder / "days.csv").open()):
                number = int(day["day"])
                entry = {**day, "day": number}
                if number < 29:
                    case = folder / f"{number:02}"
                    entry["problem"] = str((case / "problem.json").relative_to(EXP))
                    entry["problem_sha256"] = hashlib.sha256((case / "problem.json").read_bytes()).hexdigest()
                    entry["source_schedule"] = str((case / "physical_source.actions.txt").relative_to(EXP))
                record["days"].append(entry)
        print(f"{name}: {'parity/extraction complete' if not result.returncode else 'FAILED; see log'}", flush=True)
        return record

    with ThreadPoolExecutor(max_workers=2) as pool:
        records = list(pool.map(run, jobs))
    result = {"utc": datetime.now(timezone.utc).isoformat(), "source_manifest_sha256": hashlib.sha256((traces / "MANIFEST.json").read_bytes()).hexdigest(),
              "games": records, "failed_games": sum(r["exit_code"] != 0 for r in records)}
    (output / "MANIFEST.json").write_text(json.dumps(result, indent=2) + "\n")
    if result["failed_games"]:
        raise SystemExit("Some replay parity/extraction jobs failed; preserve and investigate them.")


if __name__ == "__main__":
    main()
