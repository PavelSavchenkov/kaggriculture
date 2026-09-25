"""Corpus and conditioning table for the replay-database episodes (scripts/import_replay_db.py
then build/db_trace): perspectives whose submission's latest score in the database is at least
--min-rating after at least --min-episodes database episodes, as in corpus v5. Strength and
day_index follow scripts/build_conditions.py: latest score minus the ladder reference (90th
percentile of daily top-episode ratings) on the date of the submission's last episode.

Writes data/corpus_v7db.txt (episode seat team submission split trace) and
data/conditions_v7db.csv (episode, seat, rating, day_index).
usage: build_corpus_db.py [--min-rating 2700] [--min-episodes 20]
"""
import argparse
import csv
import io
import zipfile
from datetime import date
from pathlib import Path

import numpy as np
import pyarrow.parquet as pq

from build_corpus_v5 import split_of

ROOT = Path(__file__).resolve().parents[1]
RULES_CHANGE = date(2026, 8, 15)


def day_of(timestamp):
    return date(int(timestamp[:4]), int(timestamp[5:7]), int(timestamp[8:10]))


def reference_table():
    with (ROOT / "data/daily_perspectives.csv").open() as f:
        daily = {(int(r["episode"]), int(r["seat"])) for r in csv.DictReader(f)}
    by_date = {}
    for r in csv.DictReader((ROOT / "data/perspectives_meta.csv").open()):
        if (int(r["episode"]), int(r["seat"])) in daily and r["episode_time"] and r["rating_before"]:
            by_date.setdefault(day_of(r["episode_time"]), []).append(float(r["rating_before"]))
    reference = {d: float(np.percentile(v, 90)) for d, v in sorted(by_date.items())}
    first, last = min(reference), max(reference)
    return lambda d: reference.get(min(d, last), reference[last]) if d >= first else reference[first]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--min-rating", type=float, default=2700)
    parser.add_argument("--min-episodes", type=int, default=20)
    args = parser.parse_args()
    with zipfile.ZipFile(ROOT / "data/replay_db/kaggriculture-replay-db.zip") as archive:
        index = pq.read_table(io.BytesIO(archive.read("replay_db/index/episodes_index.parquet"))).to_pylist()
    latest = {}  # submission -> (last time, score then, episodes)
    for r in index:
        for seat in (0, 1):
            sub, score = r[f"submission_id_{seat}"], r[f"score_{seat}"]
            time, old = r["create_time"], latest.get(sub, ("", None, 0))
            latest[sub] = (time, score, old[2] + 1) if time > old[0] else (old[0], old[1], old[2] + 1)
    ref = reference_table()
    kept, strengths = 0, []
    with (ROOT / "data/corpus_v7db.txt").open("w") as corpus, (ROOT / "data/conditions_v7db.csv").open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["episode", "seat", "rating", "day_index"])
        for r in csv.DictReader((ROOT / "data/replay_db/perspectives.csv").open()):
            if not (ROOT / r["trace"]).exists():
                continue  # not verified by db_trace
            last_time, score, episodes = latest[int(r["submission"])]
            if score is None or score < args.min_rating or episodes < args.min_episodes:
                continue
            episode = int(r["episode"])
            corpus.write(f"{episode} {r['seat']} {r['team']} {r['submission']} {split_of(episode)} {r['trace']}\n")
            strength = score - ref(day_of(last_time))
            strengths.append(strength)
            writer.writerow([episode, r["seat"], f"{strength:.1f}", (day_of(r["time"]) - RULES_CHANGE).days])
            kept += 1
    q = np.percentile(strengths, [1, 10, 50, 90, 99]) if strengths else []
    print(f"perspectives kept {kept}; strength quantiles 1/10/50/90/99: {np.round(q)}")


if __name__ == "__main__":
    main()
