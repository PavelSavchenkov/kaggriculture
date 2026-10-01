# One product's market per day block, ours vs M&M's recording and both opponents: units sold / average price per world.
# usage: python item_market.py <run dir> <item> (e.g. carrot)
import csv, glob, sys, collections
d, item = sys.argv[1], sys.argv[2]
BLOCKS = [(1, 8), (9, 14), (15, 21), (22, 29)]
block = lambda day: next((f"d{a}-{z}" for a, z in BLOCKS if a <= day <= z), None)
u = collections.defaultdict(float); r = collections.defaultdict(float); n = 0
for f in glob.glob(f"{d}/*.days"):
    n += 1
    for x in csv.DictReader(open(f)):
        b = block(int(x["day"]))
        if x["item"] == item and b: u[(x["who"], b)] += float(x["units"]); r[(x["who"], b)] += float(x["revenue"])
print(f"{d}: {n} worlds; {item} units / avg price per world")
for a, z in BLOCKS:
    b = f"d{a}-{z}"
    print(f"  {b:6s} " + "  ".join(f"{w} {u[(w, b)] / n:5.1f} @ {r[(w, b)] / u[(w, b)] if u[(w, b)] else 0:5.1f}" for w in ("arm", "opp", "real_arm", "real_opp")))
