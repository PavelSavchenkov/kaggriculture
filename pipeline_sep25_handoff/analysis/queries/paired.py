"""Paired comparison of Local-LB game sets (json per game): B - A in margin and win rate per
opponent and pooled, with seed-clustered 95% bootstrap CIs.
usage: paired.py <dir A> <dir B> [<dir C> ...]   (each compared with A)"""
import glob
import json
import random
import sys
from collections import defaultdict
from pathlib import Path


def load(d):
    out = {}
    for p in glob.glob(f"{d}/*.json"):
        r = json.load(open(p))
        if "error" not in r:
            out[(r["opponent"], r["seed"], r["seat"])] = r
    return out


def ci(pairs, rounds=4000):
    clusters = defaultdict(list)
    for c, v in pairs:
        clusters[c].append(v)
    keys = list(clusters)
    means = sorted(sum(sum(clusters[k]) for k in s) / sum(len(clusters[k]) for k in s)
                   for s in (random.choices(keys, k=len(keys)) for _ in range(rounds)))
    return means[int(0.025 * rounds)], means[int(0.975 * rounds)]


def row(label, keys, a, b):
    ma = [a[k]["margin"] for k in keys]
    mb = [b[k]["margin"] for k in keys]
    n = len(keys)
    d = [(k[1], b[k]["margin"] - a[k]["margin"]) for k in keys]
    w = [(k[1], float(b[k]["margin"] > 0) - float(a[k]["margin"] > 0)) for k in keys]
    lo, hi = ci(d)
    wlo, whi = ci(w)
    print(f"{label:34s} {n:4d}  A {sum(x > 0 for x in ma) / n:5.0%} {sum(ma) / n:+8,.0f}  B {sum(x > 0 for x in mb) / n:5.0%} {sum(mb) / n:+8,.0f}"
          f"  B-A {sum(v for _, v in d) / n:+7,.0f} [{lo:+,.0f}, {hi:+,.0f}]  wins {sum(v for _, v in w) / n:+.1%} [{wlo:+.1%}, {whi:+.1%}]")


def main():
    random.seed(0)
    a = load(sys.argv[1])
    for other in sys.argv[2:]:
        b = load(other)
        keys = sorted(k for k in b if k in a)
        print(f"\n== {Path(other).name} vs {Path(sys.argv[1]).name}")
        for opp in sorted({k[0] for k in keys}):
            row(opp, [k for k in keys if k[0] == opp], a, b)
        row("pooled", keys, a, b)


if __name__ == "__main__":
    main()
