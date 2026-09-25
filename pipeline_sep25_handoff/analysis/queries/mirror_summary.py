"""Mirror results: ours (X13 + probe) vs opponent (X13); seed-clustered CI of the margin."""
import csv, glob, random
from collections import defaultdict
from pathlib import Path
random.seed(0)
for f in sorted(glob.glob('mirror/*.csv')):
    rows = list(csv.DictReader(open(f)))
    if not rows: continue
    m = [(int(r['seed']), float(r['margin'])) for r in rows]
    cl = defaultdict(list)
    for s, v in m: cl[s].append(v)
    keys = list(cl); means = []
    for _ in range(4000):
        x = [v for k in random.choices(keys, k=len(keys)) for v in cl[k]]; means.append(sum(x) / len(x))
    means.sort()
    w = sum(v > 0 for _, v in m); t = sum(v == 0 for _, v in m)
    print(f"{Path(f).stem:12s} games {len(m)} wins {w} ties {t} losses {len(m) - w - t}  margin {sum(v for _, v in m)/len(m):+7.0f} [{means[100]:+.0f}, {means[3900]:+.0f}]")
