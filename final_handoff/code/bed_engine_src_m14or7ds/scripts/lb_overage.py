# Estimated overage use per Local-LB replay game (scripts/lb_play.py JSONs): sum over dawns of the
# compile time beyond the 1 s turn budget (plan string "ms N"). usage: lb_overage.py <dir> [...]
import glob, json, re, sys

rows = []
for d in sys.argv[1:]:
    for f in glob.glob(f"{d}/*.json"):
        r = json.load(open(f))
        ms = [int(m.group(1)) for x in r["dawns"] if (m := re.search(r" ms (\d+)", x.get("plan", "")))]
        rows.append((sum(max(0, m / 1000 - 1) for m in ms), r["max_action_seconds"], f.split("/")[-1]))
rows.sort(reverse=True)
print(f"{len(rows)} games; estimated overage use: max {rows[0][0]:.1f} s, mean {sum(x[0] for x in rows) / len(rows):.1f} s")
for x in rows[:3]:
    print(f"  {x[0]:.1f} s (max action {x[1]:.2f} s) {x[2]}")
