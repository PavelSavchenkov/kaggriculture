"""Our Kaggle games: episodes of one of our submissions, downloaded and converted to engine traces
(data/traces/<episode>.txt), plus a table of seat, opponent team,
opponent rank (latest CSV in data/leaderboard_sep25) and rewards.
usage: scripts/our_kaggle_games.py <submission_id> <out.csv>"""
import csv, json, sys
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(EXP / "scripts"))
import collect_replays as cr

submission, out = int(sys.argv[1]), sys.argv[2]
lb = {int(r["TeamId"]): (int(r["Rank"]), r["TeamName"], float(r["Score"]))
      for r in csv.DictReader(open(sorted((EXP / "data/leaderboard_sep25").glob("kaggriculture-publicleaderboard-*.csv"))[-1], encoding="utf-8-sig"))}
episodes = [row.to_dict() for row in cr.request(cr.api().competition_list_episodes, submission)]
json.dump(episodes, open(Path(out).with_suffix(".json"), "w"), indent=1, default=str)
rows = []
for e in episodes:
    if "COMPLETED" not in str(e["state"]) or "PUBLIC" not in str(e.get("type")):
        continue
    agents = e["agents"]
    ours = [a for a in agents if int(a["submissionId"]) == submission]
    other = [a for a in agents if int(a["submissionId"]) != submission]
    if len(ours) != 1 or len(other) != 1:
        continue
    o, t = ours[0], other[0]
    try:
        cr.fetch(int(e["id"])); cr.export(int(e["id"]))
    except Exception as error:
        print("skip", e["id"], error, flush=True)
        continue
    rank, name, score = lb.get(int(t["teamId"]), (None, str(t["teamId"]), None))
    rows.append({"episode": e["id"], "seat": o.get("index", 0), "our_reward": o.get("reward"), "opp_reward": t.get("reward"),
                 "opp_team": name, "opp_rank": rank, "opp_score": score, "opp_submission": t["submissionId"],
                 "created": e["createTime"]})
    print(rows[-1], flush=True)
with open(out, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=list(rows[0]))
    w.writeheader(); w.writerows(rows)
print("episodes", len(rows))
