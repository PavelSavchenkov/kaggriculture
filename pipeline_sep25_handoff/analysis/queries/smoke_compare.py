"""Smoke results vs X10 on the same games: Local-LB (exp/x10_trim_loss) and zoo (zoo/x10)."""
import csv, glob, json, os, sys
from pathlib import Path
def lb(d):
    out = {}
    for f in glob.glob(f'{d}/*.json'):
        r = json.load(open(f))
        if 'error' not in r: out[(r['opponent'], r['seed'], r['seat'])] = r['margin']
    return out
def zoo(d, opps):
    out = {}
    for o in opps:
        f = f'{d}/zoo_{o}.csv' if Path(f'{d}/zoo_{o}.csv').exists() else f'{d}/{o}.csv'
        if Path(f).exists():
            for r in csv.DictReader(open(f)): out[(o, int(r['seed']), int(r['seat']))] = float(r['margin'])
    return out
base_lb, base_zoo = lb(os.environ.get('BASE_LB', 'exp/x10_trim_loss')), zoo(os.environ.get('BASE_ZOO', 'zoo/x10'), ['zoo_dsm', 'v13_w384b'])
bname = os.environ.get('BASE_NAME', 'X10')
for name in sys.argv[1:]:
    for kind, cur, base in (('LB', lb(f'smoke/{name}/lb'), base_lb), ('zoo', zoo(f'smoke/{name}', ['zoo_dsm', 'v13_w384b']), base_zoo)):
        keys = [k for k in cur if k in base]
        if not keys: continue
        d = [cur[k] - base[k] for k in keys]
        print(f"{name:18s} {kind:3s} n={len(keys):3d}  wins {sum(cur[k] > 0 for k in keys)}/{len(keys)} ({bname} {sum(base[k] > 0 for k in keys)})  "
              f"margin {sum(cur[k] for k in keys)/len(keys):+8.0f} ({bname} {sum(base[k] for k in keys)/len(keys):+8.0f})  diff {sum(d)/len(d):+7.0f}  better/worse {sum(x > 0 for x in d)}/{sum(x < 0 for x in d)}")
