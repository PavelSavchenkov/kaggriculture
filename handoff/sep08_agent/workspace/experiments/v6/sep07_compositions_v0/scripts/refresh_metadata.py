"""Map a fresh leaderboard cohort to exact replay seats for C++ formatting."""
import argparse
import csv
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]
RESEARCH=EXP/"research/refresh_0314"


def main():
    parser=argparse.ArgumentParser();parser.add_argument("--research-dir",type=Path,default=RESEARCH)
    args=parser.parse_args();research=args.research_dir.resolve();assert research.is_relative_to(EXP)
    rows=list(csv.DictReader((research/"top_replay_manifest.csv").open()))
    rows.sort(key=lambda r:(int(r["rank"]),int(r["episode_id"])))
    sources=[]
    for row in rows:
        episode=int(row["episode_id"])
        replay=json.loads((EXP/f"replays/episode-{episode}-replay.json").read_text())
        replay_team=row.get("replay_team",row["team"])
        names=replay["info"]["TeamNames"];assert names.count(replay_team)==1
        seat=names.index(replay_team)
        sources.append({"program":len(sources),"id":f"episode_{episode}_seat_{seat}","episode":episode,"seat":seat,
            "team":row["team"],"replay_team":replay_team,"team_id":int(row["team_id"]),"rank":int(row["rank"]),"submission":int(row["submission_id"]),"leaderboard_snapshot":row["leaderboard_snapshot"]})
    (research/"PROGRAM_SOURCES.json").write_text(json.dumps(sources,indent=2)+"\n")
    print("fresh programs",len(sources))


if __name__=="__main__":main()
