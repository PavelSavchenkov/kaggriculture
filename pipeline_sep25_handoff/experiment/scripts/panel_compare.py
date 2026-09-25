"""Paired comparison of two evaluation panels (scripts/panel.sh): per opponent, win rate
and mean margin of A and B on the games both played, and B - A in margin and in win rate
(the ladder rating counts only wins) with seed-clustered 95% bootstrap CIs; then pooled
over groups (Local-LB, extra, C++, replays) and overall.
usage: panel_compare.py reports/panel/A reports/panel/B
"""
import csv
import glob
import json
import random
import sys
from collections import defaultdict
from pathlib import Path


def load(panel):
    games = {}  # (group, opponent, key) -> (seed cluster, margin)
    for group in ("lb", "extra"):
        for path in glob.glob(f"{panel}/{group}/*.json"):
            r = json.load(open(path))
            if "error" not in r:
                games[(group, r["opponent"], (r["seed"], r["seat"]))] = (r["seed"], r["margin"])
    for path in glob.glob(f"{panel}/cpp_*.csv"):
        opponent = Path(path).stem[4:]
        for r in csv.DictReader(open(path)):
            games[("cpp", opponent, (int(r["seed"]), int(r["seat"])))] = (int(r["seed"]), float(r["margin"]))
    replay = Path(panel) / "replay.csv"
    if replay.exists():
        for r in csv.DictReader(replay.open()):
            games[("replay", "top10", (r["trace"], r["replayed_seat"]))] = (r["trace"], float(r["margin"]))
    return games


def ci(pairs, rounds=2000):
    clusters = defaultdict(list)
    for cluster, value in pairs:
        clusters[cluster].append(value)
    keys = list(clusters)
    means = []
    for _ in range(rounds):
        sample = [v for k in random.choices(keys, k=len(keys)) for v in clusters[k]]
        means.append(sum(sample) / len(sample))
    means.sort()
    return means[int(0.025 * rounds)], means[int(0.975 * rounds)]


def row(label, keys, a, b):
    n = len(keys)
    ma, mb = [a[k][1] for k in keys], [b[k][1] for k in keys]
    lo, hi = ci([(b[k][0], b[k][1] - a[k][1]) for k in keys])
    wlo, whi = ci([(b[k][0], float(b[k][1] > 0) - float(a[k][1] > 0)) for k in keys])
    wins = (sum(x > 0 for x in mb) - sum(x > 0 for x in ma)) / n
    print(f"{label:48s} {n:4d}  A {sum(x > 0 for x in ma) / n:5.0%} {sum(ma) / n:+9,.0f}  "
          f"B {sum(x > 0 for x in mb) / n:5.0%} {sum(mb) / n:+9,.0f}  B-A {(sum(mb) - sum(ma)) / n:+8,.0f} [{lo:+,.0f}, {hi:+,.0f}]"
          f"  wins {wins:+.1%} [{wlo:+.1%}, {whi:+.1%}]")


def main():
    random.seed(0)
    a, b = load(sys.argv[1]), load(sys.argv[2])
    common = [k for k in b if k in a]
    print(f"{'opponent':48s} {'n':>4s}  A: win, margin       B: win, margin       paired B-A [95% CI]")
    for group in ("lb", "extra", "cpp", "replay"):
        keys = [k for k in common if k[0] == group]
        if not keys:
            continue
        for opponent in sorted({k[1] for k in keys}):
            row(f"{group}:{opponent}", [k for k in keys if k[1] == opponent], a, b)
        row(f"== {group} pooled", keys, a, b)
    row("== all pooled", common, a, b)


if __name__ == "__main__":
    main()
