"""Per-game ledger of our Kaggle games against strong opponents: our revenue minus the opponent's by
product (wheat_gross: sales only; wheat traders buy and resell), wages (Fibonacci per day from the audited
worker counts) and herd at day 12, for
games against opponents ranked <= RANK; mean over losses and over wins.
Needs build_dev/sales_audit and build_dev/farm_audit.
usage: scripts/kaggle_loss_table.py <games.csv from our_kaggle_games.py> [rank=30]"""
import collections, csv, statistics as st, subprocess, sys, tempfile
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
NAMES = ["wheat_gross", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]
FIB = [1, 1]
while len(FIB) < 40:
    FIB.append(FIB[-1] + FIB[-2])

games = [r for r in csv.DictReader(open(sys.argv[1])) if r["opp_rank"] and int(r["opp_rank"]) <= int(sys.argv[2] if len(sys.argv) > 2 else 30)]
tmp = Path(tempfile.mkdtemp())
with open(tmp / "list.txt", "w") as f:
    for g in games:
        s = int(g["seat"])
        f.write(f"data/traces/{g['episode']}.txt {s} ours_{g['episode']}\ndata/traces/{g['episode']}.txt {1 - s} opp_{g['episode']}\n")
env = {"SALES_ROWS": str(tmp / "rows.csv")}
subprocess.run([str(EXP / "build_dev/sales_audit"), str(tmp / "list.txt"), str(tmp / "sales.csv")], cwd=EXP, check=True,
               env={**__import__("os").environ, **env})
subprocess.run([str(EXP / "build_dev/farm_audit"), str(tmp / "list.txt"), str(tmp / "farm.csv")], cwd=EXP, check=True,
               stdout=subprocess.DEVNULL)
revenue = collections.defaultdict(lambda: [0.0] * 9)
for r in csv.DictReader(open(tmp / "rows.csv")):
    revenue[r["label"]][int(r["product"])] += int(r["units"]) * float(r["price"])
farm = collections.defaultdict(dict)
for r in csv.DictReader(open(tmp / "farm.csv")):
    farm[r["label"]][int(r["day"])] = r


def wages(label):
    days = farm[label]
    return sum(sum(FIB[:max(0, int(days[d]["workers"]) - 1)]) for d in range(1, 30) if d in days)


rows = []
for g in games:
    e = g["episode"]; o, t = f"ours_{e}", f"opp_{e}"
    margin = float(g["our_reward"]) - float(g["opp_reward"])
    row = {"episode": e, "opp": g["opp_team"][:14], "rank": int(g["opp_rank"]), "margin": margin}
    for p, n in enumerate(NAMES):
        row[n] = revenue[o][p] - revenue[t][p]
    row["wages"] = -(wages(o) - wages(t))
    row["herd12"] = "/".join(farm[o].get(12, {}).get(k, "?") for k in ("goose", "cow", "sheep")) + " vs " + \
        "/".join(farm[t].get(12, {}).get(k, "?") for k in ("goose", "cow", "sheep"))
    rows.append(row)
cols = ["margin"] + NAMES + ["wages"]
print(f"{'episode':10} {'opp':14} {'rank':>4} " + " ".join(f"{c[:6]:>7}" for c in cols) + "  herd12 (g/c/s)")
for r in sorted(rows, key=lambda r: r["margin"]):
    print(f"{r['episode']:10} {r['opp']:14} {r['rank']:4} " + " ".join(f"{r[c]:7.0f}" for c in cols) + f"  {r['herd12']}")
for label, sel in (("losses", [r for r in rows if r["margin"] < 0]), ("wins", [r for r in rows if r["margin"] > 0])):
    if sel:
        print(f"mean over {len(sel)} {label}: " + " ".join(f"{c} {st.mean(r[c] for r in sel):+.0f}" for c in cols))
