# Per-hour product flows, days first..last: run A's arm vs run B's arm (same worlds, same live opponent), per world summed over days.
# made = harvested / collected that hour, dep = reached the shed (shed change + sold), sold, pockets (carried at the hour), and the
# opponent's sales. Hour bands of 3. usage: python flow_read.py <run A> <run B> [first last] [products e.g. 0,3,5,6,7]
import csv, glob, os, sys, collections
A, B = sys.argv[1], sys.argv[2]; first, last = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) > 4 else (0, 9)
prods = [int(p) for p in sys.argv[5].split(",")] if len(sys.argv) > 5 else list(range(9))
NAMES = ["wheat", "carrot", "tomato", "straw", "melon", "egg", "milk", "wool", "fert"]
def load(run):
    out = {}
    for f in glob.glob(f"{run}/*.sell"):
        if not os.path.exists(f[:-5] + ".csv"): continue
        rows = collections.defaultdict(dict)
        for x in csv.DictReader(open(f)):
            d = int(x["day"])
            if first <= d <= last + 1 and x["who"] in ("arm", "opp"): rows[(x["who"], int(x["product"]))][d * 24 + int(x["hour"])] = x
        a = collections.defaultdict(float)
        for (who, p), hrs in rows.items():
            for t, x in hrs.items():
                d, h = divmod(t, 24)
                if d > last: continue
                b = h // 3
                a[(who, p, b, "sold")] += float(x["sold"]); a[(who, p, b, "made")] += float(x["made"]); a[(who, p, b, "pockets")] += float(x["pockets"])
                if t + 1 in hrs: a[(who, p, b, "dep")] += float(hrs[t + 1]["shed"]) - float(x["shed"]) + float(x["sold"])
        out[os.path.basename(f)] = a
    return out
LA, LB = load(A), load(B); ids = [i for i in LA if i in LB]; n = max(len(ids), 1)
print(f"{A} vs {B}: {len(ids)} worlds, days {first}-{last}, per world; each cell A / B; bands of 3 hours from h0")
for p in prods:
    for k, who in (("made", "arm"), ("dep", "arm"), ("sold", "arm"), ("pockets", "arm"), ("sold", "opp")):
        ta = sum(LA[i][(who, p, b, k)] for i in ids for b in range(8)) / n; tb = sum(LB[i][(who, p, b, k)] for i in ids for b in range(8)) / n
        if ta < 0.5 and tb < 0.5: continue
        print(f"  {NAMES[p]:6s} {who} {k:7s} tot {ta:6.1f}/{tb:<6.1f} " + " ".join(f"{sum(LA[i][(who, p, b, k)] for i in ids) / n:5.1f}/{sum(LB[i][(who, p, b, k)] for i in ids) / n:<5.1f}" for b in range(8)))
