"""Frozen-replay opponents: recent held-out episodes of the current top Kaggle teams.

For each team ranked <= --top on the public leaderboard, up to --per-team of its most
recent test-split episodes (split rule of build_corpus_v5.py) since --since, played by
the team's --submissions most recently seen submissions. Held out: validation and test
episodes. Writes lines "trace replayed_seat team".
"""
import argparse
import csv
from pathlib import Path

from build_corpus_v5 import split_of

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--top", type=int, default=10)
    parser.add_argument("--per-team", type=int, default=16)
    parser.add_argument("--since", default="2026-09-18")
    parser.add_argument("--submissions", type=int, default=2)
    parser.add_argument("--out", default="data/replay_opponents.txt")
    args = parser.parse_args()
    rows = [r for r in csv.DictReader((ROOT / "data/perspectives_meta.csv").open())
            if r["team_rank"] and int(r["team_rank"]) <= args.top and r["episode_time"] >= args.since
            and (ROOT / "data/traces" / f"{r['episode']}.txt").exists() and split_of(int(r["episode"])) != "train"]
    seen = {}
    for r in rows:
        key = (r["team_name"], r["submission_id"])
        seen[key] = max(seen.get(key, ""), r["episode_time"])
    lines = []
    for team in sorted({t for t, _ in seen}):
        subs = sorted((s for t, s in seen if t == team), key=lambda s: seen[(team, s)], reverse=True)[:args.submissions]
        games = sorted((r for r in rows if r["submission_id"] in subs), key=lambda r: r["episode_time"], reverse=True)
        for r in games[:args.per_team]:
            lines.append(f"data/traces/{r['episode']}.txt {r['seat']} {team.replace(' ', '_')}\n")
        print(team, len(games[:args.per_team]))
    (ROOT / args.out).write_text("".join(lines))
    print(len(lines), "games")


if __name__ == "__main__":
    main()
