"""Paired zoo comparison: candidate vs base on identical (opponent, seed, seat), seed-clustered CIs."""
import csv, glob, random
from collections import defaultdict
from pathlib import Path
random.seed(0)
def load(c):
    out = {}
    for f in glob.glob(f'zoo/{c}/*.csv'):
        for r in csv.DictReader(open(f)):
            out[(Path(f).stem, int(r['seed']), int(r['seat']))] = float(r['margin'])
    return out
def ci(pairs, rounds=4000):
    cl = defaultdict(list)
    for k, v in pairs: cl[k].append(v)
    keys = list(cl); ms = []
    for _ in range(rounds):
        s = [v for k in random.choices(keys, k=len(keys)) for v in cl[k]]; ms.append(sum(s) / len(s))
    ms.sort(); return ms[int(.025 * rounds)], ms[int(.975 * rounds)]
base = load('base')
for c in ('x10', 'x12'):
    b = load(c); keys = [k for k in b if k in base and k[0] != 'cand_v12_vadim6']
    d = [(k[1], b[k] - base[k]) for k in keys]; w = [(k[1], float(b[k] > 0) - float(base[k] > 0)) for k in keys]
    lo, hi = ci(d); wl, wh = ci(w)
    print(f"{c} vs base, 6 zoo opponents, {len(keys)} games: margin {sum(v for _, v in d)/len(d):+,.0f} [{lo:+,.0f}, {hi:+,.0f}]  wins {sum(v for _, v in w)/len(w):+.1%} [{wl:+.1%}, {wh:+.1%}]")
