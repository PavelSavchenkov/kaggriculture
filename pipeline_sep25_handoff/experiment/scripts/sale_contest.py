"""Sale contests in lb_play games recorded with LB_ACTIONS=1: per product and player, units
requested for sale, value at the quote before the step, mean hour, and for same-hour sales of the
same product which player held the earlier order slot. Also money gain per day and player.
usage: sale_contest.py game.json [game.json ...]
"""
import json
import sys
from collections import defaultdict

PRODUCTS = ["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER"]

for path in sys.argv[1:]:
    r = json.load(open(path))
    ours = r["seat"]
    units, value, hours = defaultdict(float), defaultdict(float), defaultdict(float)
    earlier, same = defaultdict(lambda: [0, 0, 0]), 0
    for h in r["hours"]:
        slots = [{}, {}]
        for p in (0, 1):
            for k, order in enumerate(h["market"][p]):
                if order[0] == "SELL" and order[1] in PRODUCTS:
                    slots[p][order[1]] = (k, order[2])
                    units[p, order[1]] += order[2]
                    value[p, order[1]] += order[2] * h["prices"][order[1]]
                    hours[p, order[1]] += order[2] * ((h["step"] - 1) % 24)
        for item in set(slots[0]) & set(slots[1]):
            same += 1
            a, b = slots[ours][item][0], slots[1 - ours][item][0]
            earlier[item][0 if a < b else 1 if a > b else 2] += 1
    print(f"{path}: margin {r['margin']:+.0f}, same-hour contests {same}")
    print(f"  {'product':11s} {'our units':>9s} {'rival':>6s} {'our $/u':>8s} {'rival':>6s} {'our hour':>8s} {'rival':>6s}  slot ours-first/rival-first/tie")
    for item in PRODUCTS:
        u0, u1 = units[ours, item], units[1 - ours, item]
        if u0 + u1 == 0:
            continue
        pu = lambda p, u: value[p, item] / u if u else 0
        hr = lambda p, u: hours[p, item] / u if u else 0
        print(f"  {item:11s} {u0:9.0f} {u1:6.0f} {pu(ours, u0):8.1f} {pu(1 - ours, u1):6.1f} {hr(ours, u0):8.1f} "
              f"{hr(1 - ours, u1):6.1f}  {earlier[item]}")
    days = defaultdict(lambda: [0.0, 0.0])
    prev = None
    for h in r["hours"]:
        if prev is not None:
            d = (h["step"] - 2) // 24
            for p in (0, 1):
                days[d][p] += h["money"][p] - prev[p]
        prev = h["money"]
    print("  money gain per day (ours - rival):", " ".join(f"{d}:{days[d][ours] - days[d][1 - ours]:+.0f}" for d in sorted(days)))
