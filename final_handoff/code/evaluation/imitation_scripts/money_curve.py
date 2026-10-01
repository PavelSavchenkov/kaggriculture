# Money curve from full-game hand-overs (runs/handover/mf_x<X>): M&M's recorded play until dawn X, then our copy; X=30 = M&M's
# recording played live to the end. Paired per world vs mf_x30: our final money (net of loans), the opponent's, and the margin.
# A drop between hand-over days X1 < X2 = the money our own play loses (vs M&M's recorded play) on days X1..X2-1.
# usage: python money_curve.py <handover dir> X ...
import csv, glob, sys, math
d = sys.argv[1]; xs = [int(v) for v in sys.argv[2:]]
def load(x):
    out = {}
    for f in glob.glob(f"{d}/mf_x{x}/*.csv"):
        r = next(csv.DictReader(open(f))); out[r["trace"]] = (float(r["arm"]), float(r["opp"]))
    return out
ref = load(30)
def stat(v):
    n = len(v); m = sum(v) / n; se = math.sqrt(sum((a - m) ** 2 for a in v) / (n - 1) / n) if n > 1 else 0
    return f"{m:+8.0f} (SE {se:5.0f})"
print(f"vs M&M's recording played live (mf_x30, {len(ref)} worlds): ours - M&M per world")
for x in xs:
    run = load(x); w = [t for t in run if t in ref]
    if len(w) < 2: print(f"  X={x}: {len(w)} paired"); continue
    own = [run[t][0] - ref[t][0] for t in w]; opp = [run[t][1] - ref[t][1] for t in w]
    print(f"  hand-over dawn {x:2d}: n {len(w):2d}  margin {stat([a - b for a, b in zip(own, opp)])}  own {stat(own)}  opp {stat(opp)}")
