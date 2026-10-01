"""Corpus v5: every verified trace perspective whose submission reached a final rating of
at least --min-rating (Meta Kaggle, data/perspectives_meta.csv) after at least
--min-episodes episodes, plus the v4 corpus (data/corpus.txt). Same whole-episode split
rule as v4 ("sep24-BC-opus", 80/10/10).

Writes data/corpus_v5.txt (episode seat team_id submission split trace, as v4).
Conditioning inputs are built separately (scripts/build_conditions.py).
"""
import argparse
import csv
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def split_of(episode):
    bucket = int.from_bytes(hashlib.sha256(f"sep24-BC-opus:{episode}".encode()).digest()[:8], "little") % 10
    return "test" if bucket == 0 else "validation" if bucket == 1 else "train"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--min-rating", type=float, default=2700)
    parser.add_argument("--min-episodes", type=int, default=20)
    parser.add_argument("--out", default="data/corpus_v5.txt")
    args = parser.parse_args()
    rows = {}
    with (ROOT / "data/perspectives_meta.csv").open() as f:
        for r in csv.DictReader(f):
            key = (int(r["episode"]), int(r["seat"]))
            rating = float(r["submission_latest_rating"]) if r["submission_latest_rating"] else 0.0
            known = rating and int(r["submission_episodes"]) >= args.min_episodes
            if known and rating >= args.min_rating and (ROOT / "data/traces" / f"{key[0]}.txt").exists():
                rows[key] = (int(r["team_id"]), int(r["submission_id"]))
    v4 = 0
    with (ROOT / "data/corpus.txt").open() as f:
        for line in f:
            episode, seat, team, submission = map(int, line.split()[:4])
            v4 += (episode, seat) not in rows
            rows.setdefault((episode, seat), (team, submission))
    counts = {}
    with (ROOT / args.out).open("w") as out:
        for (episode, seat), (team, submission) in sorted(rows.items()):
            split = split_of(episode)
            counts[split] = counts.get(split, 0) + 1
            out.write(f"{episode} {seat} {team} {submission} {split} data/traces/{episode}.txt\n")
    print(f"{len(rows)} perspectives ({v4} only in v4); {counts}")


if __name__ == "__main__":
    main()
