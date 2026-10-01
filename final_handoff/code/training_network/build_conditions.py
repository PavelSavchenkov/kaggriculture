"""Rating/recency conditioning inputs per perspective (data/conditions_v5.csv).

Ladder ratings drift a lot (the 90th percentile of the daily top episodes fell from
3,106 on Aug 15 to ~2,830 on Aug 31 and rose to ~3,160 by mid-September), so a raw
rating is not comparable across dates. Strength here is relative:
    strength = submission final rating - reference(date of its last episode),
reference = 90th percentile of rating_before over the official daily top-episode
datasets (data/daily_perspectives.csv) on that date, carried forward after the last
daily date. Submissions with fewer than --min-episodes episodes get no strength.
day_index = replay date as days since 2026-08-15 (the last rules change).

Columns: episode, seat, rating (strength, empty if unknown), day_index.
"""
import argparse
import csv
from datetime import date
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
RULES_CHANGE = date(2026, 8, 15)


def day_of(timestamp):
    return date(int(timestamp[:4]), int(timestamp[5:7]), int(timestamp[8:10]))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--min-episodes", type=int, default=20)
    parser.add_argument("--corpus", default="data/corpus_v5.txt")
    parser.add_argument("--out", default="data/conditions_v5.csv")
    args = parser.parse_args()
    with (ROOT / "data/daily_perspectives.csv").open() as f:
        daily = {(int(r["episode"]), int(r["seat"])) for r in csv.DictReader(f)}
    meta = list(csv.DictReader((ROOT / "data/perspectives_meta.csv").open()))
    by_date = {}
    for r in meta:
        if (int(r["episode"]), int(r["seat"])) in daily and r["episode_time"] and r["rating_before"]:
            by_date.setdefault(day_of(r["episode_time"]), []).append(float(r["rating_before"]))
    reference = {d: float(np.percentile(v, 90)) for d, v in sorted(by_date.items())}
    last = max(reference)

    def ref(d):
        return reference.get(min(d, last), reference[last]) if d >= min(reference) else reference[min(reference)]

    corpus = {tuple(map(int, line.split()[:2])) for line in (ROOT / args.corpus).open()}
    strengths = []
    with (ROOT / args.out).open("w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["episode", "seat", "rating", "day_index"])
        for r in meta:
            key = (int(r["episode"]), int(r["seat"]))
            if key not in corpus:
                continue
            strength = ""
            if r["submission_latest_rating"] and int(r["submission_episodes"]) >= args.min_episodes and r["submission_last_episode"]:
                strength = float(r["submission_latest_rating"]) - ref(day_of(r["submission_last_episode"]))
                strengths.append(strength)
                strength = f"{strength:.1f}"
            day = (day_of(r["episode_time"]) - RULES_CHANGE).days if r["episode_time"] else ""
            writer.writerow([*key, strength, day])
    q = np.percentile(strengths, [1, 10, 50, 90, 99])
    print(f"reference dates {min(reference)}..{last}; {len(strengths)} with strength; quantiles 1/10/50/90/99: {q.round(0)}")


if __name__ == "__main__":
    main()
