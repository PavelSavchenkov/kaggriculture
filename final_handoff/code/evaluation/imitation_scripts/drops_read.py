# Day compiler drops for our agent (runs with DC11_INTENTLOG / DC11_DAYLOG / DC11_LANDHOUR + DUEL_OPS / DUEL_HERD): per day block, summed
# per world: the network's crop / animal ask (pre), the intent after decode / compiler trims (final), executed plantings / placements,
# the funding fallback level (0 keep all, 1 no land, 2 no new entities, 3 no collection, 4 survival only), dropped (unfinished) stops,
# land asked vs bought. usage: python drops_read.py <dir>
import csv, glob, sys, re, collections
d = sys.argv[1]
BL = [(0, 2), (3, 5), (6, 9), (10, 14), (15, 19), (20, 24), (25, 29)]
bl = lambda x: next(f"d{a}-{z}" for a, z in BL if a <= x <= z)
a = collections.defaultdict(float); n = 0
for lst in glob.glob(f"{d}/*.lst"):
    base = lst[:-4]
    if not glob.glob(base + ".csv"): continue
    seat = int(open(lst).read().split()[1]); n += 1
    for line in open(base + ".err"):
        m = re.match(r"intent p(\d) d(\d+) money \S+ pre (.*) final (.*) status (\S+) fallback (\S+) dropped (\S+) trims (\S+)", line)
        if m and int(m.group(1)) == seat:
            day = int(m.group(2)); b = bl(day); pre = [int(v) for v in m.group(3).split()]; fin = [int(v) for v in m.group(4).split()]
            a[(b, "crop_pre")] += sum(pre[:5]); a[(b, "crop_fin")] += sum(fin[:5]); a[(b, "ani_pre")] += sum(pre[5:8]); a[(b, "ani_fin")] += sum(fin[5:8])
            a[(b, "land_ask")] += fin[8]; a[(b, f"fb{m.group(6)}")] += 1; a[(b, "dropped")] += int(m.group(7)); a[(b, "days")] += 1
        m = re.match(r"landhour p(\d) d(\d+) ask (\d+) hour (-?\d+)", line)
        if m and int(m.group(1)) == seat and int(m.group(4)) >= 0: a[(bl(int(m.group(2))), "land_bought")] += 1
    for x in csv.DictReader(open(base + ".ops")):
        if x["who"] == "arm" and x["kind"] == "plant" and int(x["what"]) < 5: a[(bl(int(x["day"])), "crop_exec")] += float(x["n"])
    prev = None
    for x in csv.DictReader(open(base + ".herd")):
        if x["who"] != "arm": continue
        cur = sum(float(x[k]) for k in ("geese", "cows", "sheep"))
        if prev is not None and int(x["day"]) >= 1:
            a[(bl(int(x["day"]) - 1), "ani_placed")] += max(0.0, cur - prev)
        prev = cur
print(f"{d}: {n} worlds; per world per block: crops ask / after trims / planted; animals ask / after trims / placed (net of escapes); land ask / bought;")
print("  fallback days (1 no land, 2 no new entities, 3 no collection, 4 survival) and dropped stops")
for lo, hi in BL:
    b = f"d{lo}-{hi}"; g = lambda k: a[(b, k)] / max(n, 1)
    print(f"  {b:7s} crops {g('crop_pre'):5.1f} / {g('crop_fin'):5.1f} / {g('crop_exec'):5.1f}   animals {g('ani_pre'):4.1f} / {g('ani_fin'):4.1f} / {g('ani_placed'):4.1f}"
          f"   land {g('land_ask'):.2f} / {g('land_bought'):.2f}   fallback 1: {g('fb1'):.2f} 2: {g('fb2'):.2f} 3: {g('fb3'):.2f} 4: {g('fb4'):.2f}   dropped {g('dropped'):5.2f}")
