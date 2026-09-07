"""Offline replay ingestion: dated proposals and exact days, with full lineage."""
import argparse
import csv
import gzip
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
RESEARCH = EXP / "research"
ITEMS = "WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP".split()


def rows(name):
    with (RESEARCH / name).open() as stream:
        return list(csv.DictReader(stream))


def main():
    global RESEARCH
    parser=argparse.ArgumentParser()
    parser.add_argument("--research-dir",type=Path,default=RESEARCH)
    args=parser.parse_args()
    RESEARCH=args.research_dir.resolve()
    assert RESEARCH.is_relative_to(EXP)
    lives = defaultdict(list)
    for name in ("crop_instances.csv", "animal_instances.csv"):
        for row in rows(name):
            lives[(int(row["episode_id"]), row["team"])].append(row)
    manifest = {(int(r["episode_id"]), r["team"]): r for r in rows("top_replay_manifest.csv")}
    games = rows("game_summary.csv")
    by_episode = defaultdict(list)
    proposals = []
    for game in games:
        episode, seat = int(game["episode_id"]), int(game["seat"])
        key = (episode, game["team"])
        source = manifest[key]
        grouped = Counter()
        instances = []
        for row in lives[key]:
            # Keep the last occupied day, including an intraday harvest/escape.
            # Precise boundaries stay in instances for compilation and diagnosis.
            start = int(row["origin_day"])
            end = min(30, int(row["end_day"]) + 1)
            item = ITEMS.index(row["name"])
            grouped[(item, start, end)] += 1
            instances.append({"item": item, "x": int(row["x"]), "y": int(row["y"]),
                              "start_day": start, "end_day_exclusive": end,
                              "start_state": int(row["start_state"]), "end_state": int(row["end_state"]),
                              "censored": bool(int(row["censored"])), "outcome": row["outcome"]})
        proposal = {"id": f"episode_{episode}_seat_{seat}", "episode": episode, "seat": seat,
                    "team": game["team"], "rank": int(game["rank"]), "submission": int(source["submission_id"]),
                    "leaderboard_snapshot": source["leaderboard_snapshot"], "cash": float(game["reward"]),
                    "opponent_cash": float(game["opponent_reward"]),
                    "cohorts": [{"item": i, "count": n, "start_day": a, "end_day_exclusive": b}
                                for (i,a,b),n in sorted(grouped.items())], "instances": instances}
        proposals.append(proposal)
        by_episode[episode].append(proposal)
    (RESEARCH / "compositions.json").write_text(json.dumps({
        "format_version": 1, "items": ITEMS,
        "day_convention": "zero-based start inclusive, end exclusive; include intraday exit day",
        "interpretation": "Observed realized lifecycles, not proof of intended policy or branches",
        "proposals": proposals,
    }, separators=(",", ":")) + "\n")
    with (RESEARCH / "compositions.txt").open("w") as out:
        out.write(f"{len(proposals)}\n")
        for proposal in proposals:
            out.write(f'{proposal["episode"]} {proposal["seat"]} {proposal["rank"]} {proposal["cash"]} {len(proposal["cohorts"])}\n')
            for cohort in proposal["cohorts"]:
                out.write(" ".join(str(cohort[k]) for k in ("item", "count", "start_day", "end_day_exclusive")) + "\n")
    directory = RESEARCH / "day_templates"
    directory.mkdir(exist_ok=True)
    records = []
    for episode, selected in sorted(by_episode.items()):
        replay_path = EXP / "replays" / f"episode-{episode}-replay.json"
        replay = json.loads(replay_path.read_text())
        source_hash = hashlib.sha256(replay_path.read_bytes()).hexdigest()
        for proposal in selected:
            seat = proposal["seat"]
            days = []
            for day in range(30):
                start, end = day*24, min((day+1)*24, 719)
                actions = [replay["steps"][step+1][seat]["action"] for step in range(start, end)]
                days.append({"day": day, "start_observation": replay["steps"][start][seat]["observation"],
                             "actions": actions,
                             "end_observation": replay["steps"][end][seat]["observation"]})
            target = directory / f'{proposal["id"]}.json.gz'
            payload = {"source": {k: proposal[k] for k in ("id", "episode", "seat", "team", "rank", "submission", "leaderboard_snapshot")},
                       "replay_sha256": source_hash,
                       "usage": "Exact recorded days; start-state compatibility or complete schedule repair is required before reuse",
                       "days": days}
            with gzip.open(target, "wt", encoding="utf-8") as stream:
                json.dump(payload, stream, separators=(",", ":"))
            records.append({"id": proposal["id"], "path": str(target.relative_to(EXP)),
                            "sha256": hashlib.sha256(target.read_bytes()).hexdigest(),
                            "days": 30, "actions": sum(len(d["actions"]) for d in days)})
    report = {"proposals": len(proposals), "cohorts": sum(len(p["cohorts"]) for p in proposals),
              "instances": sum(len(p["instances"]) for p in proposals),
              "templates": records}
    (RESEARCH / "composition_template_manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: v for k,v in report.items() if k != "templates"}))


if __name__ == "__main__":
    main()
