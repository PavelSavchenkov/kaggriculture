"""Stage-1 gate of the LB screen (Imitation, Sep 30 21:45): an arm's 30 games vs #1 (m19-fc2, seeds 977000-977014, both seats) vs
m19base on the same games: PASS if its half-draw score >= m19base's or its paired margin > -300. Prints PASS / FAIL and the numbers.
usage: gatecheck.py <arm tag> [control tag=scr_m19base]"""
import glob, json, sys
import numpy as np
OPP = 'pavel-bc-opus-v17d-dc12m19-fc2-m68'
def load(tag):
    d = {}
    for f in glob.glob(f'out/{tag}/{OPP}_*.json'):
        r = json.load(open(f)); m = r.get('money')
        if m and not r.get('error'): d[(r['seed'], r['seat'])] = m[r['seat']] - m[1 - r['seat']]
    return d
a, c = load(sys.argv[1]), load(sys.argv[2] if len(sys.argv) > 2 else 'scr_m19base'); k = sorted(set(a) & set(c))
sc = lambda d: np.mean([(d[x] > 0) + 0.5 * (d[x] == 0) for x in k])
dm = np.array([a[x] - c[x] for x in k]); sa, scc = sc(a), sc(c)
seeds = sorted({x[0] for x in k}); cl = np.array([np.mean([a[x] - c[x] for x in k if x[0] == s]) for s in seeds])  # cluster by seed
ok = sa >= scc or dm.mean() > -300
print(f"{'PASS' if ok else 'FAIL'} {sys.argv[1]} vs #1 n {len(k)}: score {100 * sa:.1f}% vs m19base {100 * scc:.1f}%, paired margin {dm.mean():+.0f} (seed-clustered SE {cl.std(ddof=1) / np.sqrt(len(cl)):.0f}, {len(cl)} seeds)")
