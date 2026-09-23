"""Freeze whole-episode splits before downloading current public replays."""
import argparse
import csv
import gzip
import hashlib
import json
import threading
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path

from kaggle.api.kaggle_api_extended import KaggleApi
from requests import HTTPError, Timeout, ConnectionError

ROOT = Path(__file__).resolve().parents[1]
LOCAL = threading.local()
RATE_LOCK = threading.Lock()
NEXT_REQUEST = 0.
REQUEST_SPACING = 1.5


def wait_for_request():
    global NEXT_REQUEST
    while True:
        with RATE_LOCK:
            now = time.monotonic()
            delay = NEXT_REQUEST - now
            if delay <= 0:
                NEXT_REQUEST = now + REQUEST_SPACING
                return
        # Another worker can extend the shared cooldown while this one sleeps.
        # Recheck before dispatching; a previously reserved slot is not permission.
        time.sleep(delay)


def request(method, *args, **kwargs):
    global NEXT_REQUEST
    for attempt in range(6):
        wait_for_request()
        try:
            return method(*args, **kwargs)
        except HTTPError as error:
            if error.response.status_code not in (429, 500, 502, 503, 504) or attempt == 5:
                raise
            delay = max(float(error.response.headers.get("Retry-After", 0)), 30 * 2 ** attempt)
            print(f"HTTP {error.response.status_code}: retry in {delay}s", flush=True)
        except (Timeout, ConnectionError):
            if attempt == 5:
                raise
            delay = 15 * 2 ** attempt
            print(f"Network interruption: retry in {delay}s", flush=True)
        with RATE_LOCK:
            NEXT_REQUEST = max(NEXT_REQUEST, time.monotonic() + delay)


def api():
    if not hasattr(LOCAL, "api"):
        LOCAL.api = KaggleApi()
        LOCAL.api.authenticate()
    return LOCAL.api


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".partial")
    temporary.write_text(json.dumps(value, indent=2, ensure_ascii=False) + "\n")
    temporary.replace(path)


def query(path, method, identity):
    if not path.exists():
        save(path, [row.to_dict() for row in request(getattr(api(), method), identity)])
    return json.loads(path.read_text())


def select(team, args, excluded):
    team_id, rank = int(team["TeamId"]), int(team["Rank"])
    directory = ROOT / "data/metadata" / str(team_id)
    submissions = query(directory / "submissions.json", "competition_team_submissions", team_id)
    rows = []
    for submission in sorted(submissions, key=lambda s: float(s.get("publicScore") or -1), reverse=True):
        episodes = query(directory / f"episodes_{submission['id']}.json", "competition_list_episodes", submission["id"])
        usable = [e for e in episodes if "COMPLETED" in str(e["state"]) and
                  "PUBLIC" in str(e.get("type")) and int(e["id"]) not in excluded]
        limit = args.top_episodes if rank <= 10 else args.episodes
        for episode in sorted(usable, key=lambda e: (e["createTime"], e["id"]), reverse=True)[:limit]:
            identity = int(episode["id"])
            bucket = int.from_bytes(hashlib.sha256(f"sep23-BC-v1:{identity}".encode()).digest()[:8], "little") % 20
            split = "confirmation" if bucket == 0 else "development" if bucket < 3 else "validation" if bucket < 6 else "train"
            rows.append({"episode": identity, "team_id": team_id, "team": team["TeamName"],
                         "rank": rank, "score": float(team["Score"]), "submission": int(submission["id"]),
                         "submission_score": float(submission.get("publicScore") or 0),
                         "created": episode["createTime"], "split": split})
    print(f"selected rank={rank} team={team['TeamName']} perspectives={len(rows)}", flush=True)
    return rows


def fetch(episode):
    directory = ROOT / "data/replays"
    directory.mkdir(exist_ok=True)
    compressed = directory / f"{episode}.json.gz"
    if compressed.exists():
        return episode
    request(api().competition_episode_replay, episode, str(directory), quiet=True)
    raw = directory / f"episode-{episode}-replay.json"
    data = raw.read_bytes()
    replay = json.loads(data)
    if replay["info"]["EpisodeId"] != episode or len(replay["steps"]) != 720:
        raise ValueError(f"Incomplete replay {episode}")
    with gzip.open(compressed.with_suffix(".partial"), "wb", compresslevel=2) as stream:
        stream.write(data)
    compressed.with_suffix(".partial").replace(compressed)
    save(ROOT / "data/replay_metadata" / f"{episode}.json",
         {"episode": episode, "teams": replay["info"]["TeamNames"],
          "sha256": hashlib.sha256(data).hexdigest()})
    raw.unlink()
    return episode


def main():
    global REQUEST_SPACING, NEXT_REQUEST
    parser = argparse.ArgumentParser()
    parser.add_argument("--teams", type=int, default=20)
    parser.add_argument("--top-episodes", type=int, default=100)
    parser.add_argument("--episodes", type=int, default=40)
    parser.add_argument("--workers", type=int, default=3)
    parser.add_argument("--name", default="fresh20")
    parser.add_argument("--download", action="store_true")
    parser.add_argument('--request-spacing', type=float, default=1.5)
    parser.add_argument('--initial-cooldown', type=float, default=0.)
    args = parser.parse_args()
    if args.request_spacing < 1.5:
        raise ValueError('Request spacing must be at least 1.5 seconds')
    REQUEST_SPACING = args.request_spacing
    NEXT_REQUEST = time.monotonic() + max(0., args.initial_cooldown)
    path = ROOT / "data" / f"{args.name}_selection.json"
    if not path.exists():
        excluded = set(json.loads((ROOT / "data/excluded_episodes.json").read_text()))
        leaderboard = next((ROOT / "data/current_leaderboard").glob("*.csv"))
        with leaderboard.open(encoding="utf-8-sig") as stream:
            teams = list(csv.DictReader(stream))[:args.teams]
        with ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures = [pool.submit(select, team, args, excluded) for team in teams]
            rows = [row for future in as_completed(futures) for row in future.result()]
        # A team may have multiple selected submissions in the same episode.
        rows = list({(r["episode"], r["team_id"]): r for r in rows}.values())
        rows.sort(key=lambda r: (r["rank"], r["episode"]))
        save(path, {"utc": datetime.now(timezone.utc).isoformat(), "args": vars(args),
                    "split_rule": "episode SHA256 sep23-BC-v1: 70% train, 15% validation, 10% development, 5% confirmation",
                    "leaderboard_sha256": hashlib.sha256(leaderboard.read_bytes()).hexdigest(), "rows": rows})
    rows = json.loads(path.read_text())["rows"]
    episodes = sorted({r["episode"] for r in rows if r["split"] != "confirmation"})
    print(f"frozen {len(rows)} perspectives; {len(episodes)} downloadable episodes; confirmation unopened", flush=True)
    if args.download:
        with ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures = [pool.submit(fetch, episode) for episode in episodes]
            for number, future in enumerate(as_completed(futures), 1):
                identity = future.result()
                print(f"downloaded {number}/{len(episodes)} episode={identity}", flush=True)


if __name__ == "__main__":
    main()
