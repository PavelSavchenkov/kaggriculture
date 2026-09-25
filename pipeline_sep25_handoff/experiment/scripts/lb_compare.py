"""Paired comparison of two Local-LB result folders (same opponents and seeds).

Per opponent: wins, mean margin with a seed-clustered 95% bootstrap CI, and the
paired difference B - A (seed-clustered CI). Also fallback levels and trims per day.
usage: lb_compare.py A_dir[,A_dir2] B_dir[,B_dir2]
"""
import glob
import json
import random
import re
import sys
from collections import Counter, defaultdict


def load(directories):
    games = {}
    for path in (p for d in directories.split(",") for p in glob.glob(f"{d}/*.json")):
        r = json.load(open(path))
        if "error" not in r:
            games[(r["opponent"], r["seed"], r["seat"])] = r
    return games


def ci(values_by_seed, rounds=4000):
    seeds = list(values_by_seed)
    means = []
    for _ in range(rounds):
        sample = [v for s in random.choices(seeds, k=len(seeds)) for v in values_by_seed[s]]
        means.append(sum(sample) / len(sample))
    means.sort()
    return means[int(0.025 * rounds)], means[int(0.975 * rounds)]


def days(games):
    levels, trims = Counter(), 0
    total = 0
    for r in games.values():
        for d in r["dawns"]:
            m = re.search(r"fallback (\d+)", d["plan"])
            if not m:
                continue
            total += 1
            levels[int(m.group(1))] += 1
            trims += "trimmed" in d["plan"]
    return total, levels, trims


def main():
    random.seed(0)
    a, b = load(sys.argv[1]), load(sys.argv[2])
    for opponent in sorted({k[0] for k in b}):
        keys = sorted(k for k in b if k[0] == opponent and k in a)
        by_seed_a, by_seed_b, diff = defaultdict(list), defaultdict(list), defaultdict(list)
        for k in keys:
            by_seed_a[k[1]].append(a[k]["margin"])
            by_seed_b[k[1]].append(b[k]["margin"])
            diff[k[1]].append(b[k]["margin"] - a[k]["margin"])
        n = len(keys)
        wa = sum(a[k]["margin"] > 0 for k in keys)
        wb = sum(b[k]["margin"] > 0 for k in keys)
        ma = sum(a[k]["margin"] for k in keys) / n
        mb = sum(b[k]["margin"] for k in keys) / n
        lo, hi = ci(by_seed_b)
        dlo, dhi = ci(diff)
        print(f"{opponent}: games {n} | A wins {wa} mean {ma:,.0f} | B wins {wb} mean {mb:,.0f} [{lo:,.0f}, {hi:,.0f}]"
              f" | B-A {mb - ma:+,.0f} [{dlo:+,.0f}, {dhi:+,.0f}]")
    for name, games in (("A", a), ("B", b)):
        total, levels, trims = days(games)
        print(f"{name}: days {total} fallback {dict(sorted(levels.items()))} trimmed days {trims} ({trims / total:.1%})")


if __name__ == "__main__":
    main()
