# Opponent realism: per product and period, units sold per world, mean sale hour and dawn share (h0-5) for the live opponent (opp) and
# for M&M's real opponent in the same worlds (real_opp). usage: python opp_hours.py <run dir> ...
import csv, glob, sys, collections
P = {"0": "wheat", "3": "straw", "6": "milk", "7": "wool"}
for d in sys.argv[1:]:
    a = collections.defaultdict(float); n = 0
    for f in glob.glob(f"{d}/*.sell"):
        if not glob.glob(f[:-5] + ".csv"): continue
        n += 1
        for x in csv.DictReader(open(f)):
            if x["who"] not in ("opp", "real_opp") or x["product"] not in P: continue
            day, h, u = int(x["day"]), int(x["hour"]), float(x["sold"])
            per = "d3-9" if 3 <= day <= 9 else "d10-28" if 10 <= day <= 28 else None
            if not per or u <= 0: continue
            k = (x["who"], P[x["product"]], per)
            a[k + ("u",)] += u; a[k + ("h",)] += u * h; a[k + ("dawn",)] += u * (h <= 5)
    print(f"{d.split('/')[-1]} ({n} worlds): units / mean hour / dawn share, live opponent | M&M's real opponent")
    for per in ("d3-9", "d10-28"):
        cells = []
        for p in ("wool", "milk", "wheat", "straw"):
            o, r = ("opp", p, per), ("real_opp", p, per)
            f = lambda k: f"{a[k + ('u',)] / max(n, 1):5.1f} h{a[k + ('h',)] / max(a[k + ('u',)], 1):4.1f} {a[k + ('dawn',)] / max(a[k + ('u',)], 1):4.0%}"
            cells.append(f"{p} {f(o)} | {f(r)}")
        print(f"  {per:6s} " + "   ".join(cells))
