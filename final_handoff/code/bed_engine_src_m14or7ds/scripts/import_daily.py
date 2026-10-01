"""Import Kaggle's official daily episode datasets (kaggle/kaggriculture-episodes-<date>).

Each day holds that day's top ~600 episodes by average rating. For every date (newest
first) the zip is downloaded, each replay is converted to an engine trace in
data/traces, traces are verified against this engine (build/trace_check; mismatches are
deleted and counted), both seats are recorded in data/daily_perspectives.csv (episode,
seat, team, date, reward, avg_score), and the zip is deleted.

usage: import_daily.py [--first 2026-09-23] [--last 2026-07-30] [--workers 16]
"""
import argparse
import csv
import io
import json
import subprocess
import sys
import zipfile
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from collect_replays import export_replay  # noqa: E402

DAILY = ROOT / "data/kaggle_daily"
OUT = ROOT / "data/daily_perspectives.csv"


def convert(args):
    zip_path, member = args
    episode = int(Path(member).stem)
    destination = ROOT / "data/traces" / f"{episode}.txt"
    with zipfile.ZipFile(zip_path) as archive:
        replay = json.loads(archive.read(member))
    names = replay["info"].get("TeamNames") or [a.get("Name") for a in replay["info"]["Agents"]]
    rewards = [s.get("reward") for s in replay["steps"][-1]]
    if not destination.exists():
        try:
            export_replay(replay, destination)
        except Exception as error:  # malformed replay: record, skip
            return episode, names, rewards, f"export: {error}"
    return episode, names, rewards, ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--first", default="2026-09-23")
    parser.add_argument("--last", default="2026-07-30")
    parser.add_argument("--workers", type=int, default=16)
    args = parser.parse_args()
    with (DAILY / "index/manifest.csv").open() as stream:
        dates = sorted((r["date"] for r in csv.DictReader(stream) if args.last <= r["date"] <= args.first), reverse=True)
    done = set()
    if OUT.exists():
        with OUT.open() as stream:
            done = {r["date"] for r in csv.DictReader(stream)}
    new_file = not OUT.exists()
    with OUT.open("a", newline="") as out:
        writer = csv.writer(out)
        if new_file:
            writer.writerow(["episode", "seat", "team", "date", "reward", "avg_score"])
        for date in dates:
            if date in done:
                continue
            directory = DAILY / f"raw_{date}"
            zips = list(directory.glob("*.zip"))
            if not zips:
                subprocess.run(["kaggle", "datasets", "download", f"kaggle/kaggriculture-episodes-{date}", "-p", str(directory)],
                               check=True, capture_output=True)
                zips = list(directory.glob("*.zip"))
            zip_path = zips[0]
            with zipfile.ZipFile(zip_path) as archive:
                members = [m for m in archive.namelist() if m.endswith(".json")]
                scores = {int(r["episode_id"]): r["avg_score"]
                          for r in csv.DictReader(io.TextIOWrapper(archive.open("manifest.csv"), encoding="utf-8"))}
            with ProcessPoolExecutor(args.workers) as pool:
                results = list(pool.map(convert, [(zip_path, m) for m in members], chunksize=4))
            traces = [str(ROOT / "data/traces" / f"{e}.txt") for e, _, _, error in results if not error]
            checked = subprocess.run([str(ROOT / "build/trace_check"), *traces], capture_output=True, text=True).stdout.split("\n")
            failed = {Path(line.split()[1]).stem for line in checked if line.startswith("fail")}
            for episode in failed:
                (ROOT / "data/traces" / f"{episode}.txt").unlink(missing_ok=True)
            kept = 0
            for episode, names, rewards, error in results:
                if error or str(episode) in failed:
                    continue
                kept += 1
                for seat in (0, 1):
                    writer.writerow([episode, seat, names[seat], date, rewards[seat], scores.get(episode, "")])
            out.flush()
            print(f"{date}: episodes {len(members)} kept {kept} engine-mismatch {len(failed)} "
                  f"export-errors {sum(bool(r[3]) for r in results)}", flush=True)
            zip_path.unlink()


if __name__ == "__main__":
    main()
