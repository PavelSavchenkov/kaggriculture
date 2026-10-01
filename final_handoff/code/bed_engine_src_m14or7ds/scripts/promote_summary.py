"""Summary of scripts/promote_check.sh outputs for one or more candidates: wins and mean margin per gate.
usage: scripts/promote_summary.py reports/promote/<cand> [...]"""
import csv, glob, json, os, statistics as st, sys


def csv_games(pattern):
    rows = [r for f in sorted(glob.glob(pattern)) for r in csv.DictReader(open(f))]
    return [float(r["margin"]) for r in rows]


def line(label, margins):
    if not margins:
        return f"{label}: -"
    wins = sum(m > 0 for m in margins)
    return f"{label}: {wins}-{sum(m < 0 for m in margins)} of {len(margins)} ({wins / len(margins):.0%}), mean {st.mean(margins):+.0f}"


for d in sys.argv[1:]:
    print(f"== {os.path.basename(d)}")
    for s in ("1300", "1400"):
        print("  " + line(f"zoo (our compiler) {s}", csv_games(f"{d}/zoo_race_{s}/*.csv")))
    print("  " + line("zoo (default compiler)", csv_games(f"{d}/zoo_default/*.csv")))
    print("  " + line("head to head vs reference", csv_games(f"{d}/h2h_*.csv")))
    print("  " + line("static top-team replays", csv_games(f"{d}/replay.csv")))
    lb = [json.load(open(f))["margin"] for f in glob.glob(f"{d}/lb/*.json")]
    print("  " + line("Local-LB exact games", lb))
