"""Acquire public evidence only. Never execute notebook code."""
import argparse
import csv
import hashlib
import json
import subprocess
import zipfile
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("--teams", type=int, default=12)
    parser.add_argument("--episodes", type=int, default=6)
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()
    assert args.name.replace("_", "").isalnum()
    output = EXP / "research" / args.name
    output.mkdir(exist_ok=args.resume)
    started = datetime.now(timezone.utc).isoformat()

    def run(command, log):
        cmd = ["conda", "run", "--no-capture-output", "-n", "kaggriculture", *map(str, command)]
        with (output / log).open("w") as target:
            subprocess.run(cmd, stdout=target, stderr=subprocess.STDOUT, check=True, timeout=1200)
        return {"command": cmd, "log": log}

    with ThreadPoolExecutor(max_workers=2) as pool:
        futures = [
            pool.submit(run, ["kaggle", "competitions", "leaderboard", "kaggriculture", "-d", "-p", output], "leaderboard.log"),
            pool.submit(run, ["kaggle", "kernels", "list", "--competition", "kaggriculture", "--sort-by", "dateRun", "--page-size", "100", "--csv"], "notebooks_by_date.csv"),
        ]
        commands = [future.result() for future in futures]
    for path in output.glob("*.zip"):
        with zipfile.ZipFile(path) as archive:
            for member in archive.infolist():
                assert Path(member.filename).name == member.filename
                archive.extract(member, output)
    commands.append(run(["python", EXP / "scripts/pull_replays.py", "--research-dir", output,
                         "--teams", args.teams, "--episodes", args.episodes, "--workers", 4], "replays.log"))
    rows = list(csv.DictReader((output / "top_replay_manifest.csv").open()))
    episodes = sorted({int(row["episode_id"]) for row in rows})
    paths = sorted(p for p in output.rglob("*") if p.is_file())
    paths += [EXP / "replays" / f"episode-{episode}-replay.json" for episode in episodes]
    record = {"started_utc": started, "completed_utc": datetime.now(timezone.utc).isoformat(),
              "commands": commands, "player_games": len(rows), "episodes": episodes,
              "source_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}
    (output / "REFRESH.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({"player_games": len(rows), "unique_replays": len(episodes), "output": str(output)}), flush=True)


if __name__ == "__main__":
    main()
