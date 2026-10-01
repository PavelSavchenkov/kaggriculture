# Paired full-game read: a run dir's csv rows vs a baseline run dir's rows of the same worlds (arm = our seat, opp = our package sub).
# usage: python paired_read.py <run dir> <baseline dir>
import csv, glob, sys, math, os
run, base = sys.argv[1], sys.argv[2]
d = []
for f in sorted(glob.glob(f"{run}/*.csv")):
    b = f"{base}/{os.path.basename(f)}"
    if not os.path.exists(b): continue
    r, q = next(csv.DictReader(open(f))), next(csv.DictReader(open(b)))
    d.append((float(r["arm"]) - float(q["arm"]), float(r["opp"]) - float(q["opp"]), float(r["arm"]) > float(r["opp"]), float(q["arm"]) > float(q["opp"])))
def s(v):
    n = len(v); m = sum(v) / n; se = math.sqrt(sum((a - m) ** 2 for a in v) / (n - 1) / n) if n > 1 else 0
    return f"{m:+7.0f} (SE {se:5.0f})"
if d:
    marg = [a - b for a, b, _, _ in d]
    print(f"{run} vs {base}: n {len(d)}  margin {s(marg)}  own {s([a for a, *_ in d])}  opp {s([b for _, b, _, _ in d])}  "
          f"better / equal / worse {sum(m > 0 for m in marg)} / {sum(m == 0 for m in marg)} / {sum(m < 0 for m in marg)}  wins {sum(w for *_, w, _ in d)} vs {sum(w for *_, w in d)}")
