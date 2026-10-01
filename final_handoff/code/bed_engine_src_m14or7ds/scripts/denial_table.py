"""Per-perspective denial: how far this game pushed the opponent below its team's typical final money.
denial = mean final money of the opponent's team over all its perspectives in the corpora - the opponent's final money in this game
(both seats' final money from the trace's last line; the opponent's team from the other seat's corpus line; teams with >= 5 games).
-> data/perspectives_denial.csv (episode, seat, own, opp, margin, denial)."""
import csv, glob, statistics as st
from collections import defaultdict

seat_team, path_of = {}, {}
for corpus in sorted(glob.glob("data/corpus_*.txt")):
    for line in open(corpus):
        t = line.split()
        if len(t) < 6: continue
        key = (int(t[0]), int(t[1]))
        seat_team[key] = int(t[2]); path_of.setdefault(key, t[5])
money = {}
for (ep, seat), path in path_of.items():
    try:
        last = open(path).read().splitlines()[-1].split()
        money[(ep, seat)] = (float(last[seat]), float(last[1 - seat]))
    except Exception:
        pass
team_money = defaultdict(list)
for (ep, seat), (own, _) in money.items():
    team_money[seat_team[(ep, seat)]].append(own)
typical = {t: st.mean(v) for t, v in team_money.items() if len(v) >= 5}
rows = []
for (ep, seat), (own, opp) in money.items():
    opp_team = seat_team.get((ep, 1 - seat))
    denial = typical[opp_team] - opp if opp_team in typical else ""
    rows.append((ep, seat, own, opp, own - opp, denial))
with open("data/perspectives_denial.csv", "w", newline="") as f:
    w = csv.writer(f); w.writerow(["episode", "seat", "own", "opp", "margin", "denial"]); w.writerows(rows)
known = [r for r in rows if r[5] != ""]
print(f"perspectives {len(rows)}, denial known {len(known)}; denial mean {st.mean(r[5] for r in known):+.0f} sd {st.stdev(r[5] for r in known):.0f}")
wins = [r for r in known if r[4] > 0]
print(f"won {len(wins)}; corr(margin, denial) = {st.correlation([r[4] for r in known], [r[5] for r in known]):.2f}")
