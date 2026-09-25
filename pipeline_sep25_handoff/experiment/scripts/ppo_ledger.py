"""Money ledger of lb_play games recorded with LB_ACTIONS=1, ours minus the opponent's, per game:
sale value per product (units x quote before the step; large lots are overvalued the same way for
both sides), wages (daily Fibonacci hire cost), seeds, animals, bought wheat and fertilizer,
last-day money gain, and plant mix at dawns 20/24/27. Games sorted by margin.
usage: ppo_ledger.py game.json [game.json ...]
"""
import json
import sys
from collections import defaultdict

PRODUCTS = ["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER"]
SEED = {"WHEAT": 10, "CARROT": 20, "TOMATO": 50, "STRAWBERRY": 100, "MELON": 80}
ANIMAL = {"GOOSE": 300, "COW": 400, "SHEEP": 500}


def wages(n):
    a, b, total = 1, 1, 0
    for _ in range(n):
        total += a
        a, b = b, a + b
    return total


def ledger(r):
    out = [defaultdict(float), defaultdict(float)]
    hires = [defaultdict(int), defaultdict(int)]
    for h in r["hours"]:
        day = (h["step"] - 1) // 24
        for p in (0, 1):
            for op, item, n in ([o[0], o[1], o[2]] if len(o) == 3 else [o[0], None, 1] for o in h["market"][p]):
                if op == "SELL" and item in PRODUCTS:
                    out[p]["sell " + item] += n * h["prices"][item]
                elif op == "HIRE":
                    hires[p][day] += 1
                elif op == "BUY_SEED":
                    out[p]["seeds"] -= n * SEED[item]
                elif op == "BUY_ANIMAL":
                    out[p]["animals"] -= n * ANIMAL[item]
                elif op == "BUY_PRODUCT":
                    out[p]["buy " + item] -= n * h["prices"][item]
    for p in (0, 1):
        out[p]["wages"] = -sum(wages(n) for n in hires[p].values())
    first = next(h for h in r["hours"] if (h["step"] - 1) // 24 == 29)
    for p in (0, 1):
        out[p]["last day"] = r["money"][p] - first["money"][p]
    return out


rows = []
for path in sys.argv[1:]:
    r = json.load(open(path))
    if "hours" not in r:
        continue
    s = r["seat"]
    l = ledger(r)
    diff = {k: l[s].get(k, 0) - l[1 - s].get(k, 0) for k in set(l[0]) | set(l[1])}
    plants = {d["day"]: (d["ours"]["plants"], d["rival"]["plants"]) for d in r["dawns"]}
    rows.append((r["margin"], path.split("/")[-1], diff, plants))
keys = ["sell " + p for p in PRODUCTS] + ["seeds", "animals", "buy WHEAT", "buy FERTILIZER", "wages", "last day"]
print("margin   game                      " + " ".join(f"{k.replace('sell ', '')[:6]:>7s}" for k in keys))
for margin, name, diff, plants in sorted(rows, key=lambda x: x[0]):
    print(f"{margin:+7.0f}  {name[13:-5]:24s}  " + " ".join(f"{diff.get(k, 0):+7.0f}" for k in keys))
mean = {k: sum(r[2].get(k, 0) for r in rows) / len(rows) for k in keys}
print(f"{'mean':>7s}  {'':24s}  " + " ".join(f"{mean[k]:+7.0f}" for k in keys))
print("\nplants at dawn 20/24/27 (ours | rival) for the 4 lowest margins:")
for margin, name, diff, plants in sorted(rows, key=lambda x: x[0])[:4]:
    print(f"{margin:+.0f} {name}")
    for d in (20, 24, 27):
        if d in plants:
            print(f"   day {d}: {plants[d][0]} | {plants[d][1]}")
