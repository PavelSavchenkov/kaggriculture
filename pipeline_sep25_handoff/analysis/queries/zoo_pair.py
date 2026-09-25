"""Paired zoo comparison: B vs A on identical (opponent, seed, seat), seed-clustered CIs. usage: zoo_pair.py A B"""
import csv, glob, random, sys
from collections import defaultdict
from pathlib import Path
random.seed(0)
def load(c):
    out = {}
    for f in glob.glob(f'zoo/{c}/*.csv'):
        for r in csv.DictReader(open(f)): out[(Path(f).stem, int(r['seed']), int(r['seat']))] = float(r['margin'])
    return out
def ci(pairs, rounds=4000):
    cl = defaultdict(list)
    for k, v in pairs: cl[k].append(v)
    keys = list(cl); ms = []
    for _ in range(rounds):
        s = [v for k in random.choices(keys, k=len(keys)) for v in cl[k]]; ms.append(sum(s) / len(s))
    ms.sort(); return ms[int(.025 * rounds)], ms[int(.975 * rounds)]
a, b = load(sys.argv[1]), load(sys.argv[2])
keys = [k for k in b if k in a]
for opp in sorted({k[0] for k in keys}) + ['pooled']:
    ks = keys if opp == 'pooled' else [k for k in keys if k[0] == opp]
    d = [((k[0], k[1]), b[k] - a[k]) for k in ks]; lo, hi = ci(d)
    print(f"{opp:16s} n={len(ks):3d} {sys.argv[2]} wins {sum(b[k] > 0 for k in ks)}/{len(ks)} ({sys.argv[1]} {sum(a[k] > 0 for k in ks)})  diff {sum(v for _, v in d)/len(d):+7.0f} [{lo:+.0f}, {hi:+.0f}]")
