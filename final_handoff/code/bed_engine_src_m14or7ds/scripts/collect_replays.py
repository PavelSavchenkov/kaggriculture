"""Download fresh public replays of the current top teams and convert them to engine traces.

usage: collect_replays.py --teams 20 --per-team 40 [--download] [--export] [--name fresh] [--refresh]
--per-team is per submission; --submissions picks each team's best-scored submissions.
Selection is frozen in data/fresh_selection.json before any replay is downloaded.
Episodes already in data/traces are skipped. Split by episode SHA256:
80% train, 10% validation, 10% test (test is not opened until the end).
"""
import argparse
import array
import csv
import ctypes
import gzip
import hashlib
import io
import json
import os
import sys
import threading
import time
import zipfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path
from types import SimpleNamespace

from kaggle.api.kaggle_api_extended import KaggleApi
from requests import HTTPError, Timeout, ConnectionError

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[2]
sys.path.insert(0, str(REPO / "fast_game_engine"))
from export_trace import ITEMS, ITI, ENGINE_VERSION, ENGINE_SOURCE_HASH, canonical_values, enc_unit, enc_order

COMPETITION = "kaggriculture"
CONFIG_KEYS = ("episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn", "turnsPerDay",
               "shedCapacity", "weedSpawnChance", "townShopUnlockInterval", "townShopSellInterval",
               "townCenterSellInterval", "farmHandCostMult")
LOCAL = threading.local()
LOCK = threading.Lock()
NEXT = 0.0
SPACING = 1.5


def request(method, *args, **kwargs):
    global NEXT
    for attempt in range(6):
        while True:
            with LOCK:
                delay = NEXT - time.monotonic()
                if delay <= 0:
                    NEXT = time.monotonic() + SPACING
                    break
            time.sleep(delay)
        try:
            return method(*args, **kwargs)
        except HTTPError as error:
            if error.response.status_code not in (429, 500, 502, 503, 504) or attempt == 5:
                raise
            wait = max(float(error.response.headers.get("Retry-After", 0)), 30 * 2 ** attempt)
        except (Timeout, ConnectionError):
            if attempt == 5:
                raise
            wait = 15 * 2 ** attempt
        print(f"retry in {wait}s", flush=True)
        with LOCK:
            NEXT = max(NEXT, time.monotonic() + wait)


def api():
    if not hasattr(LOCAL, "api"):
        LOCAL.api = KaggleApi()
        LOCAL.api.authenticate()
    return LOCAL.api


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + ".partial")
    temporary.write_text(json.dumps(value, indent=1, ensure_ascii=False) + "\n")
    temporary.replace(path)


def query(path, method, identity):
    if not path.exists():
        save(path, [row.to_dict() for row in request(getattr(api(), method), identity)])
    return json.loads(path.read_text())


def split_of(episode):
    bucket = int.from_bytes(hashlib.sha256(f"sep24-BC-opus:{episode}".encode()).digest()[:8], "little") % 10
    return "test" if bucket == 0 else "validation" if bucket == 1 else "train"


def leaderboard():
    directory = ROOT / os.environ.get("LEADERBOARD_DIR", "data/leaderboard")  # LEADERBOARD_DIR: a fresher download
    directory.mkdir(parents=True, exist_ok=True)
    if not list(directory.glob("*.csv")):
        request(api().competition_leaderboard_download, COMPETITION, str(directory))
        for archive in directory.glob("*.zip"):
            with zipfile.ZipFile(archive) as z:
                z.extractall(directory)
    path = next(directory.glob("*.csv"))
    with path.open(encoding="utf-8-sig") as stream:
        return list(csv.DictReader(stream)), hashlib.sha256(path.read_bytes()).hexdigest()


def select(team, args, known):
    team_id, rank = int(team["TeamId"]), int(team["Rank"])
    directory = ROOT / "data/metadata" / str(team_id)
    submissions = query(directory / "submissions.json", "competition_team_submissions", team_id)
    rows = []
    for submission in sorted(submissions, key=lambda s: float(s.get("publicScore") or -1), reverse=True)[:args.submissions]:
        episodes = query(directory / f"episodes_{submission['id']}.json", "competition_list_episodes", submission["id"])
        usable = [e for e in episodes if "COMPLETED" in str(e["state"]) and "PUBLIC" in str(e.get("type"))
                  and int(e["id"]) not in known]
        for episode in sorted(usable, key=lambda e: (e["createTime"], e["id"]), reverse=True)[:args.per_team]:
            seats = [int(a.get("index", 0)) for a in episode["agents"]
                     if int(a["teamId"]) == team_id and int(a["submissionId"]) == int(submission["id"])]
            if len(seats) != 1:
                continue
            rows.append({"episode": int(episode["id"]), "team_id": team_id, "team": team["TeamName"], "rank": rank,
                         "score": float(team["Score"]), "submission": int(submission["id"]),
                         "submission_score": float(submission.get("publicScore") or 0), "seat": seats[0],
                         "created": episode["createTime"], "split": split_of(int(episode["id"]))})
    print(f"selected rank={rank} team={team['TeamName']} perspectives={len(rows)}", flush=True)
    return rows


def fetch(episode):
    directory = ROOT / "data/replays"
    directory.mkdir(parents=True, exist_ok=True)
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
    raw.unlink()
    return episode


def export(episode):
    destination = ROOT / "data/traces" / f"{episode}.txt"
    if destination.exists():
        return episode
    with gzip.open(ROOT / "data/replays" / f"{episode}.json.gz", "rb") as stream:
        replay = json.load(stream)
    export_replay(replay, destination)
    return episode


def export_replay(replay, destination):
    """Writes one Kaggle replay as an engine trace (actions plus per-step parity hashes)."""
    config = replay["configuration"]
    if config["episodeSteps"] != 720 or len(replay["steps"]) != 720:
        raise ValueError(f"Unsupported episode length: {destination.stem}")
    lines = [f"{replay['info']['seed']} 719", "CONFIG " + " ".join(str(config[k]) for k in CONFIG_KEYS),
             f"ENGINE {ENGINE_VERSION} {ENGINE_SOURCE_HASH}"]
    for frame in replay["steps"][1:]:
        for player in frame:
            action = player.get("action") or {}
            units = [action.get("farmer") or ["PASS"], *(action.get("hands") or [])]
            orders = (action.get("market") or [])[:config["maxMarketOrdersPerTurn"]]
            words = [len(units), len(orders)]
            for unit in units:
                words.extend(enc_unit(unit))
            for order in orders:
                words.extend(enc_order(order))
            lines.append(" ".join(map(str, words)))
    library = ctypes.CDLL(str(ROOT / "build/libtrace_hash.so"))
    library.hash_words.argtypes = [ctypes.POINTER(ctypes.c_int64), ctypes.c_size_t]
    library.hash_words.restype = ctypes.c_uint64
    lines.append("TRUTH")
    for index, frame in enumerate(replay["steps"]):
        wrapped = []
        for player in frame:
            observation = player["observation"]
            for field in ("inventory", "prices"):
                observation["market"][field] = {p: observation["market"][field][p] for p in ITEMS[:9]}
            private = observation["private"]
            private["shed"] = {p: private["shed"].get(p, 0) for p in ITEMS}
            private["seeds"] = {p: private["seeds"].get(p, 0) for p in ITEMS[:5]}
            private["inventories"] = [dict(sorted(inv.items(), key=lambda pair: ITI[pair[0]]))
                                      for inv in private["inventories"]]
            wrapped.append(SimpleNamespace(observation=SimpleNamespace(**observation), status=player["status"]))
        values = array.array("q", canonical_values(index, wrapped))
        buffer = (ctypes.c_int64 * len(values)).from_buffer(values)
        checksum = library.hash_words(buffer, len(values))
        observation = frame[0]["observation"]
        words = [int(farm["money"]) for farm in observation["farms"]]
        words += list(observation["market"]["inventory"].values()) + [checksum]
        lines.append(" ".join(map(str, words)))
    temporary = destination.with_suffix(".partial")
    temporary.write_text("\n".join(lines) + "\n")
    temporary.replace(destination)


def main():
    global SPACING
    parser = argparse.ArgumentParser()
    parser.add_argument("--teams", type=int, default=20)
    parser.add_argument("--submissions", type=int, default=2)
    parser.add_argument("--per-team", type=int, default=40)
    parser.add_argument("--workers", type=int, default=3)
    parser.add_argument("--download", action="store_true")
    parser.add_argument("--export", action="store_true")
    parser.add_argument("--name", default="fresh", help="selection file data/<name>_selection.json")
    parser.add_argument("--refresh", action="store_true", help="re-list episodes (ignore cached listings)")
    args = parser.parse_args()
    path = ROOT / "data" / f"{args.name}_selection.json"
    if args.refresh:
        for listing in (ROOT / "data/metadata").glob("*/episodes_*.json"):
            listing.unlink()
    if not path.exists():
        known = {int(p.stem) for p in (ROOT / "data/traces").glob("*.txt")}
        teams, board_hash = leaderboard()
        with ThreadPoolExecutor(max_workers=args.workers) as pool:
            futures = [pool.submit(select, team, args, known) for team in teams[:args.teams]]
            rows = [row for future in as_completed(futures) for row in future.result()]
        rows = list({(r["episode"], r["team_id"]): r for r in rows}.values())
        rows.sort(key=lambda r: (r["rank"], r["episode"]))
        save(path, {"utc": datetime.now(timezone.utc).isoformat(), "args": vars(args), "leaderboard_sha256": board_hash,
                    "split_rule": "episode SHA256 sep24-BC-opus: 80% train, 10% validation, 10% test", "rows": rows})
    rows = json.loads(path.read_text())["rows"]
    episodes = sorted({r["episode"] for r in rows})
    print(f"frozen {len(rows)} perspectives, {len(episodes)} episodes", flush=True)
    if args.download:
        with ThreadPoolExecutor(max_workers=args.workers) as pool:
            order = sorted(episodes, reverse=bool(os.environ.get("NEWEST_FIRST")))  # NEWEST_FIRST: latest episodes first
            for number, future in enumerate(as_completed([pool.submit(fetch, e) for e in order]), 1):
                print(f"downloaded {number}/{len(episodes)} {future.result()}", flush=True)
    if args.export:
        done = [e for e in episodes if (ROOT / "data/replays" / f"{e}.json.gz").exists()]
        with ThreadPoolExecutor(max_workers=8) as pool:
            for number, future in enumerate(as_completed([pool.submit(export, e) for e in done]), 1):
                print(f"exported {number}/{len(done)} {future.result()}", flush=True)


if __name__ == "__main__":
    main()
