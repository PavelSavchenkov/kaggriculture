"""Paired day-window split of two duel runs on shared worlds: mean change (A - B) in our money and the opponent's money gained within
each window, from the csv's cumulative arm_dX / opp_dX columns (dawn money of day X). usage: day_split.py <run A> <run B> [bounds 0,6,10,18]"""
import csv, glob, math, os, sys
A, B = sys.argv[1], sys.argv[2]
bounds = [int(x) for x in (sys.argv[3] if len(sys.argv) > 3 else "0,6,10,18").split(",")]
def load(d):
    out = {}
    for f in glob.glob(f"{d}/*.csv"):
        for r in csv.DictReader(open(f)):
            out[os.path.basename(f)] = {k: float(v) for k, v in r.items() if k.startswith(("arm_d", "opp_d")) or k in ("arm", "opp")}
    return out
a, b = load(A), load(B); keys = sorted(set(a) & set(b)); n = len(keys)
def stat(xs):
    m = sum(xs) / len(xs); se = math.sqrt(sum((x - m) ** 2 for x in xs) / (len(xs) - 1) / len(xs)) if len(xs) > 1 else float("nan")
    return f"{m:+7.0f} ({se:4.0f})"
last = max(int(k[5:]) for k in a[keys[0]] if k.startswith("arm_d"))
edges = bounds + [last]
print(f"{A} - {B}, n {n}: window  own  opp  (money gained within the window, SE)")
for lo, hi in zip(edges[:-1], edges[1:]):
    own = [(a[k][f"arm_d{hi}"] - a[k][f"arm_d{lo}"]) - (b[k][f"arm_d{hi}"] - b[k][f"arm_d{lo}"]) for k in keys]
    opp = [(a[k][f"opp_d{hi}"] - a[k][f"opp_d{lo}"]) - (b[k][f"opp_d{hi}"] - b[k][f"opp_d{lo}"]) for k in keys]
    print(f"  d{lo:2d}-{hi:2d}  own {stat(own)}  opp {stat(opp)}")
own = [(a[k]["arm"] - a[k][f"arm_d{last}"]) - (b[k]["arm"] - b[k][f"arm_d{last}"]) for k in keys]
opp = [(a[k]["opp"] - a[k][f"opp_d{last}"]) - (b[k]["opp"] - b[k][f"opp_d{last}"]) for k in keys]
print(f"  d{last}-end  own {stat(own)}  opp {stat(opp)}")
