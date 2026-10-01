# First divergence: per day, run A minus run B on the same worlds (arm seat and live opponent), mean per world with SE; both runs vs
# the same live opponent. Dawn cash, spend and sales per hour band, hires, plantings per crop, dawn herd. Marks |mean| > 2 SE with *.
# usage: python first_div.py <run A> <run B> [first last]
import csv, glob, os, sys, math, collections
A, B = sys.argv[1], sys.argv[2]; first, last = (int(sys.argv[3]), int(sys.argv[4])) if len(sys.argv) > 4 else (0, 10)
CROPS = {0: "wheat", 1: "carrot", 2: "tomato", 3: "straw", 4: "melon"}; BANDS = ["h0-2", "h3-11", "h12-20", "h21-23"]
def load(run, i):
    a = collections.defaultdict(float)
    r = next(csv.DictReader(open(f"{run}/{i}.csv")))
    for d in range(25):
        a[(d, "arm", "cash")] = float(r[f"arm_d{d}"]); a[(d, "opp", "cash")] = float(r[f"opp_d{d}"])
    for x in csv.DictReader(open(f"{run}/{i}.days")):
        d, w = int(x["day"]), x["who"]
        if w not in ("arm", "opp"): continue
        if x["item"] == "spend": a[(d, w, "spend " + BANDS[int(x["band"])])] += float(x["revenue"])
        elif x["item"] == "hires": a[(d, w, "hires")] += float(x["units"])
        else:
            a[(d, w, f"sold {x['item']} units")] += float(x["units"]); a[(d, w, f"sold {x['item']} $")] += float(x["revenue"])
            a[(d, w, "sales " + BANDS[int(x["band"])])] += float(x["revenue"])
    if os.path.exists(f"{run}/{i}.ops"):
        for x in csv.DictReader(open(f"{run}/{i}.ops")):
            if x["kind"] == "plant" and int(x["what"]) in CROPS and x["who"] in ("arm", "opp"):
                a[(int(x["day"]), x["who"], "plant " + CROPS[int(x["what"])])] += float(x["n"])
    if os.path.exists(f"{run}/{i}.herd"):
        for x in csv.DictReader(open(f"{run}/{i}.herd")):
            if x["who"] in ("arm", "opp"):
                for k in ("geese", "cows", "sheep", "plants"): a[(int(x["day"]), x["who"], "dawn " + k)] = float(x[k])
    return a
ids = [os.path.basename(f)[:-4] for f in glob.glob(f"{A}/*.csv") if os.path.exists(f"{B}/{os.path.basename(f)}")]
LA, LB = {i: load(A, i) for i in ids}, {i: load(B, i) for i in ids}; n = len(ids)
keys = sorted({k[1:] for i in ids for k in list(LA[i]) + list(LB[i])})
print(f"{A} minus {B}: {n} worlds, per world (* = |mean| > 2 SE); only rows with |mean| >= 0.3 units / $30")
for d in range(first, last + 1):
    out = []
    for who, k in keys:
        v = [LA[i][(d, who, k)] - LB[i][(d, who, k)] for i in ids]
        m = sum(v) / n; se = math.sqrt(sum((x - m) ** 2 for x in v) / (n - 1) / n) if n > 1 else 0
        big = abs(m) >= (30 if ("$" in k or "cash" in k or "spend" in k or "sales" in k) else 0.3)
        if big: out.append(f"{who} {k} {m:+.1f}{'*' if se and abs(m) > 2 * se else ''}")
    print(f" day {d:2d}: " + "; ".join(out))
