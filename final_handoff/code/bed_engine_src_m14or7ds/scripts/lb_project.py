"""Projected Local-LB table if a challenger joined: the snapshot's own Bradley-Terry fit (lb.elo.fit_ratings) over all games among
its active agents plus the challenger's exact games (scripts/lb_play.py result folders; margin > 0 = win, < 0 = loss, 0 = draw), then
its active cap. usage: lb_project.py <snapshot dir> <challenger id> <result dir> [...]"""
import glob, json, sys
from pathlib import Path

snap, me, dirs = Path(sys.argv[1]), sys.argv[2], sys.argv[3:]
sys.path.insert(0, str(snap / "src"))
from lb.config import load_config
from lb.elo import fit_ratings
from lb.matrix import games_for_active, load_matrix

config = load_config(snap / "config/leaderboard.yaml")
rankings = json.load(open(snap / "data/rankings.json"))
agents = rankings["agents"]
active = sorted(k for k, v in agents.items() if v.get("active"))
games = games_for_active(load_matrix(snap / config.match.matrix_path), active)
mine = []
for d in dirs:
    for f in sorted(glob.glob(f"{d}/*.json")):
        r = json.load(open(f))
        if r["opponent"] not in active:
            continue
        result = "win" if r["margin"] > 0 else "loss" if r["margin"] < 0 else "draw"
        mine.append({"challenger": me, "opponent": r["opponent"], "seed": r["seed"], "seat": r["seat"], "result": result})
ratings = {a: float(config.elo.initial_rating) for a in active + [me]}
fitted = fit_ratings(ratings, games + mine, config)
record = {a: [0, 0] for a in active + [me]}
for g in games + mine:
    w = g["result"] == "win"
    record[g["challenger"]][0 if w else 1] += 1
    record[g["opponent"]][1 if w else 0] += 1
order = sorted(fitted, key=lambda a: -fitted[a])
cap = getattr(config.leaderboard, "max_active", None) if hasattr(config, "leaderboard") else None
print(f"challenger games: {len(mine)} ({sum(g['result'] == 'win' for g in mine)} won); snapshot games: {len(games)}; cap {cap}")
for i, a in enumerate(order, 1):
    print(f"{i:2d} {fitted[a]:7.1f} {record[a][0]:3d}-{record[a][1]:3d} {a}{'  <- challenger' if a == me else ''}")
