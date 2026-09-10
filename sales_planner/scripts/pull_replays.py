"""Read-only Kaggle retrieval. Python stops at the offline data boundary."""
import argparse
import csv
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path


EXPERIMENT = Path(__file__).resolve().parents[1]
RESEARCH = EXPERIMENT / "research"
REPLAYS = EXPERIMENT / "replays"


def kaggle(*arguments):
    result = subprocess.run(
        ["conda", "run", "-n", "kaggriculture", "kaggle", *map(str, arguments)],
        check=True, capture_output=True, text=True, timeout=120,
    )
    return result.stdout


def query(*arguments):
    return json.JSONDecoder().raw_decode(kaggle(*arguments).lstrip())[0]


def select(leader, count, snapshot):
    team = leader["TeamId"]
    submissions = query("competitions", "team-submissions", team, "--format", "json")
    (RESEARCH / f"team_{team}_submissions.json").write_text(json.dumps(submissions, indent=2))
    submission = max(submissions, key=lambda row: float(row["publicScore"]))
    episodes = query("competitions", "episodes", submission["id"], "--format", "json")
    (RESEARCH / f"submission_{submission['id']}_episodes.json").write_text(json.dumps(episodes, indent=2))
    completed = sorted(
        (episode for episode in episodes if episode["state"] == "EpisodeState.COMPLETED"),
        key=lambda episode: episode["createTime"], reverse=True,
    )[:count]
    if not completed:
        raise RuntimeError(f"no completed games for {leader['TeamName']}")
    print(f"rank {leader['Rank']} {leader['TeamName']}: {len(completed)} games", flush=True)
    return [{
        "rank": int(leader["Rank"]), "team_id": int(team), "team": leader["TeamName"],
        "leaderboard_score": float(leader["Score"]), "leaderboard_snapshot": snapshot,
        "submission_id": int(submission["id"]),
        "submission_score_at_query": float(submission["publicScore"]),
        "submission_date": submission["dateSubmitted"], "episode_id": int(episode["id"]),
        "episode_create_time": episode["createTime"],
    } for episode in completed]


def download(episode):
    path = REPLAYS / f"episode-{episode}-replay.json"
    if not path.exists():
        kaggle("competitions", "replay", episode, "-p", REPLAYS, "-q")
    if not path.exists() or path.stat().st_size == 0:
        raise RuntimeError(f"empty or missing replay {episode}")
    return episode, path.stat().st_size


def main():
    global RESEARCH
    parser = argparse.ArgumentParser()
    parser.add_argument("--teams", type=int, default=12)
    parser.add_argument("--episodes", type=int, default=6)
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--research-dir",type=Path,default=RESEARCH)
    parser.add_argument("--ranks",type=int,nargs="+")
    args = parser.parse_args()
    RESEARCH=args.research_dir.resolve()
    assert RESEARCH.is_relative_to(EXPERIMENT)
    leaderboard = max(RESEARCH.glob("kaggriculture-publicleaderboard-*.csv"))
    with leaderboard.open(encoding="utf-8-sig", newline="") as source:
        available=list(csv.DictReader(source))
        leaders = [r for r in available if int(r["Rank"]) in args.ranks] if args.ranks else available[:args.teams]
    rows = []
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = [executor.submit(select, leader, args.episodes, leaderboard.name) for leader in leaders]
        for future in as_completed(futures):
            rows.extend(future.result())
        rows.sort(key=lambda row: (row["rank"], -row["episode_id"]))
        with (RESEARCH / "top_replay_selection.csv").open("w", newline="") as target:
            writer = csv.DictWriter(target, fieldnames=rows[0])
            writer.writeheader()
            writer.writerows(rows)
        episodes = sorted({row["episode_id"] for row in rows})
        futures = [executor.submit(download, episode) for episode in episodes]
        sizes = {}
        for index, future in enumerate(as_completed(futures), 1):
            episode, size = future.result()
            sizes[episode] = size
            if index % 8 == 0 or index == len(futures):
                print(f"downloaded {index}/{len(futures)}", flush=True)
    for row in rows:
        row["replay_bytes"] = sizes[row["episode_id"]]
    with (RESEARCH / "top_replay_manifest.csv").open("w", newline="") as target:
        writer = csv.DictWriter(target, fieldnames=rows[0])
        writer.writeheader()
        writer.writerows(rows)


if __name__ == "__main__":
    main()
