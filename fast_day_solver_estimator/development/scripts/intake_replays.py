"""Read-only Kaggle intake with whole-team reserve groups fixed before downloads."""
import argparse
import csv
import hashlib
import json
import subprocess
import zipfile
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path


EXP = Path(__file__).resolve().parents[1]
INTAKE = EXP / "data/replay_intake"


def command(*args):
    result = subprocess.run(["conda", "run", "-n", "kaggriculture", "kaggle", *map(str, args)],
                            capture_output=True, text=True, check=True, timeout=150)
    return result.stdout


def query(*args):
    return json.JSONDecoder().raw_decode(command(*args).lstrip())[0]


def initialize():
    manifest = INTAKE / "FAMILY_SPLITS.json"
    if manifest.exists():
        return json.loads(manifest.read_text())
    with zipfile.ZipFile(INTAKE / "kaggriculture.zip") as archive:
        for member in archive.infolist():
            if Path(member.filename).name != member.filename:
                raise ValueError("unexpected leaderboard archive path")
            archive.extract(member, INTAKE)
    leaderboard = max(INTAKE.glob("kaggriculture-publicleaderboard-*.csv"))
    rows = list(csv.DictReader(leaderboard.open(encoding="utf-8-sig")))[:32]
    groups = ["development", "validation", "holdout_a", "holdout_b"]
    data = {"utc": datetime.now(timezone.utc).isoformat(), "leaderboard": leaderboard.name,
            "leaderboard_sha256": hashlib.sha256(leaderboard.read_bytes()).hexdigest(),
            "selection_rule": "Top 32 teams; rank modulo 4 assigns development, validation, holdout_a, holdout_b.",
            "caveat": "Team split precedes downloading new cases. Some historical development sources may share teams; audit overlap and quarantine affected final teams before any test. Test sources are unused by the new estimator until their declared gate.",
            "teams": [{"rank": int(r["Rank"]), "team_id": int(r["TeamId"]), "name": r["TeamName"],
                       "split": groups[(int(r["Rank"]) - 1) % 4]} for r in rows]}
    manifest.write_text(json.dumps(data, indent=2) + "\n")
    return data


def fetch(team, output, episodes):
    team_id = team["team_id"]
    submissions = query("competitions", "team-submissions", team_id, "--format", "json")
    (output / f"team_{team_id}_submissions.json").write_text(json.dumps(submissions, indent=2) + "\n")
    if not submissions:
        return {**team, "status": "no_submissions", "episodes": []}
    submission = max(submissions, key=lambda row: float(row["publicScore"]))
    games = query("competitions", "episodes", submission["id"], "--format", "json")
    (output / f"submission_{submission['id']}_episodes.json").write_text(json.dumps(games, indent=2) + "\n")
    selected = sorted((g for g in games if g["state"] == "EpisodeState.COMPLETED"),
                      key=lambda g: g["createTime"], reverse=True)[:episodes]
    result = {**team, "status": "downloaded", "submission_id": submission["id"], "episodes": []}
    for game in selected:
        episode = int(game["id"])
        path = output / f"episode-{episode}-replay.json"
        if not path.exists():
            command("competitions", "replay", episode, "-p", output, "-q")
        data = path.read_bytes()
        if not data:
            raise ValueError(f"empty replay {episode}")
        result["episodes"].append({"episode_id": episode, "created": game["createTime"],
                                    "file": path.name, "sha256": hashlib.sha256(data).hexdigest()})
    print(f"{team['split']} rank {team['rank']}: {len(selected)} episodes downloaded", flush=True)
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("split", choices=["metadata", "development", "validation", "holdout_a", "holdout_b"])
    parser.add_argument("--episodes", type=int, default=4)
    parser.add_argument("--round", default="round1")
    args = parser.parse_args()
    data = initialize()
    if args.split == "metadata":
        print(json.dumps({"teams": len(data["teams"]), "splits": sorted({t["split"] for t in data["teams"]})}))
        return
    output = INTAKE / f"{args.split}_{args.round}"
    output.mkdir(exist_ok=False)
    teams = [t for t in data["teams"] if t["split"] == args.split]
    with ThreadPoolExecutor(max_workers=3) as pool:
        results = list(pool.map(lambda team: fetch(team, output, args.episodes), teams))
    manifest = {"utc": datetime.now(timezone.utc).isoformat(), "split": args.split, "teams": results,
                "source_split_sha256": hashlib.sha256((INTAKE / "FAMILY_SPLITS.json").read_bytes()).hexdigest()}
    (output / "DOWNLOAD.json").write_text(json.dumps(manifest, indent=2) + "\n")


if __name__ == "__main__":
    main()
