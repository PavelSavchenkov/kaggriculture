"""Head-to-head read vs a fixed opponent: per arm dir, W-D-L of the arm against the opponent (margin sign) and the paired margin vs a
base dir on the shared worlds.  usage: h2h_wdl.py <base dir> <arm dir> [<arm dir> ...]"""
import csv, glob, math, os, sys


def load(d):
    rows = {}
    for f in glob.glob(f'{d}/*.csv'):
        for r in csv.DictReader(open(f)):
            rows[(r['trace'], r['arm_seat'])] = float(r['margin'])
    return rows


base = load(sys.argv[1])
for d in sys.argv[2:]:
    arm = load(d)
    keys = [k for k in arm if k in base]
    if not keys:
        continue
    w = sum(arm[k] > 0 for k in keys); t = sum(arm[k] == 0 for k in keys); l = len(keys) - w - t
    bw = sum(base[k] > 0 for k in keys); bt = sum(base[k] == 0 for k in keys)
    diff = [arm[k] - base[k] for k in keys]
    n = len(diff); mean = sum(diff) / n
    se = math.sqrt(sum((x - mean) ** 2 for x in diff) / (n - 1) / n) if n > 1 else float('nan')
    score = (w + 0.5 * t) / n
    print(f'{os.path.basename(d):18s} n {n:3d}  vs opp W-D-L {w}-{t}-{l} score {score:.2f} (base {bw}-{bt}-{n - bw - bt})'
          f'  paired margin {mean:+7.0f} (SE {se:5.0f})')
