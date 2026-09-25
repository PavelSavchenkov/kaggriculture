"""Frozen replays: probe vs base, paired per game, split into top-10 replays and our Kaggle games."""
import csv, random, sys
random.seed(0)
base = {r['trace']: r for r in csv.DictReader(open('frozen/base.csv'))}
for n in sys.argv[1:]:
    rows = list(csv.DictReader(open(f'frozen/{n}.csv')))
    for part, f in [('top10', lambda t: 'kaggle_ours' not in t), ('kaggle', lambda t: 'kaggle_ours' in t)]:
        sel = [r for r in rows if f(r['trace'])]
        diffs = [float(r['margin']) - float(base[r['trace']]['margin']) for r in sel]
        own = [float(r['own']) - float(base[r['trace']]['own']) for r in sel]
        wins = sum(float(r['margin']) > 0 for r in sel); bw = sum(float(base[r['trace']]['margin']) > 0 for r in sel)
        bs = sorted(sum(random.choices(diffs, k=len(diffs))) / len(diffs) for _ in range(4000))
        print(f"{n:12s} {part:6s} n={len(diffs)} margin {sum(diffs)/len(diffs):+7.0f} [{bs[100]:+.0f}, {bs[3900]:+.0f}] own {sum(own)/len(own):+6.0f} wins {wins} (base {bw})")
