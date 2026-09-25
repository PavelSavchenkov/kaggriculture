"""Import raw Kaggle replays (*.json.gz) from another local collection: convert each to an
engine trace, verify it against this engine (build/trace_check), and record both seats
in data/local_perspectives.csv (episode, seat, team, source, reward).

usage: import_local.py <replay_dir> [--workers 8]
"""
import argparse
import csv
import gzip
import json
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))
from collect_replays import export_replay  # noqa: E402

OUT = ROOT / "data/local_perspectives.csv"


def convert(path):
    episode = int(Path(path).name.split(".")[0])
    destination = ROOT / "data/traces" / f"{episode}.txt"
    with gzip.open(path, "rb") as stream:
        replay = json.load(stream)
    names = replay["info"].get("TeamNames") or [a.get("Name") for a in replay["info"]["Agents"]]
    rewards = [s.get("reward") for s in replay["steps"][-1]]
    if not destination.exists():
        try:
            export_replay(replay, destination)
        except Exception as error:
            return episode, names, rewards, f"export: {error}"
    return episode, names, rewards, ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("replay_dir", type=Path)
    parser.add_argument("--workers", type=int, default=8)
    args = parser.parse_args()
    paths = sorted(str(p) for p in args.replay_dir.glob("*.json.gz"))
    with ProcessPoolExecutor(args.workers) as pool:
        results = list(pool.map(convert, paths, chunksize=8))
    traces = [str(ROOT / "data/traces" / f"{e}.txt") for e, _, _, error in results if not error]
    failed = set()
    for start in range(0, len(traces), 500):
        out = subprocess.run([str(ROOT / "build/trace_check"), *traces[start:start + 500]], capture_output=True, text=True).stdout
        failed |= {Path(line.split()[1]).stem for line in out.split("\n") if line.startswith("fail")}
    for episode in failed:
        (ROOT / "data/traces" / f"{episode}.txt").unlink(missing_ok=True)
    new_file = not OUT.exists()
    kept = 0
    with OUT.open("a", newline="") as out:
        writer = csv.writer(out)
        if new_file:
            writer.writerow(["episode", "seat", "team", "source", "reward"])
        for episode, names, rewards, error in results:
            if error or str(episode) in failed:
                continue
            kept += 1
            for seat in (0, 1):
                writer.writerow([episode, seat, names[seat], str(args.replay_dir), rewards[seat]])
    print(f"replays {len(paths)} kept {kept} engine-mismatch {len(failed)} export-errors {sum(bool(r[3]) for r in results)}")


if __name__ == "__main__":
    main()
