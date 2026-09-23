"""Convert downloaded replays to the engine's independently checked trace format."""
import argparse
import array
import ctypes
import gzip
import hashlib
import json
import sys
from functools import lru_cache
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor, as_completed
from pathlib import Path
from types import SimpleNamespace

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parents[2] / "fast_game_engine"))
from export_trace import ITEMS, ITI, ENGINE_VERSION, ENGINE_SOURCE_HASH, canonical_values, enc_unit, enc_order

CONFIG_KEYS = ("episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn", "turnsPerDay",
               "shedCapacity", "weedSpawnChance", "townShopUnlockInterval", "townShopSellInterval",
               "townCenterSellInterval", "farmHandCostMult")


@lru_cache(maxsize=None)
def episode_metadata(team, submission):
    path = ROOT / 'data/metadata' / str(team) / f'episodes_{submission}.json'
    return {int(row['id']): row for row in json.loads(path.read_text())}


def resolve_seat(row, episode, replay_names):
    agents = episode['agents']
    # The API omits protobuf's default index=0. Names can change after a replay.
    if len(agents) != 2 or {int(a.get('index', 0)) for a in agents} != {0, 1}:
        raise ValueError(f"Invalid episode seats: {row['episode']}")
    matched = [a for a in agents if int(a['teamId']) == row['team_id'] and
               int(a['submissionId']) == row['submission']]
    if len(matched) != 1:
        raise ValueError(f"Ambiguous episode identity: {row['episode']}")
    seat = int(matched[0].get('index', 0))
    if row['team'] in replay_names and replay_names.index(row['team']) != seat:
        raise ValueError(f"Replay name and stable identity disagree: {row['episode']}")
    return seat


def export(episode):
    destination = ROOT / "data/traces" / f"{episode}.txt"
    if destination.exists():
        return episode
    with gzip.open(ROOT / "data/replays" / f"{episode}.json.gz", "rb") as stream:
        replay = json.load(stream)
    config = replay["configuration"]
    if config["episodeSteps"] != 720 or len(replay["steps"]) != 720:
        raise ValueError(f"Unsupported episode length: {episode}")
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
    return episode


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--selection", default="fresh20_selection.json")
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--available-only", action="store_true")
    parser.add_argument("--processes", action="store_true")
    args = parser.parse_args()
    rows = json.loads((ROOT / "data" / args.selection).read_text())["rows"]
    rows = [r for r in rows if r["split"] != "confirmation"]
    if args.available_only:
        rows = [r for r in rows if (ROOT / "data/replays" / f"{r['episode']}.json.gz").exists()]
    (ROOT / "data/traces").mkdir(exist_ok=True)
    episodes = sorted({r["episode"] for r in rows})
    executor = ProcessPoolExecutor if args.processes else ThreadPoolExecutor
    with executor(max_workers=args.workers) as pool:
        futures = [pool.submit(export, episode) for episode in episodes]
        for number, future in enumerate(as_completed(futures), 1):
            print(f"exported {number}/{len(episodes)} episode={future.result()}", flush=True)
    for row in rows:
        metadata = json.loads((ROOT / "data/replay_metadata" / f"{row['episode']}.json").read_text())
        episode = episode_metadata(row['team_id'], row['submission'])[row['episode']]
        row['seat'] = resolve_seat(row, episode, metadata['teams'])
        row['seat_source'] = 'episode_team_and_submission_ids'
        row['replay_team'] = metadata['teams'][row['seat']]
        if row['replay_team'] != row['team']:
            with gzip.open(ROOT / 'data/replays' / f"{row['episode']}.json.gz", 'rb') as stream:
                replay = json.load(stream)
            for agent in episode['agents']:
                seat = int(agent.get('index', 0))
                if replay['steps'][-1][seat]['reward'] != agent['reward']:
                    raise ValueError(f"Renamed team replay reward mismatch: {row['episode']}")
            print(f"renamed episode={row['episode']} seat={row['seat']} replay={row['replay_team']!r} current={row['team']!r}", flush=True)
        row["trace"] = f"data/traces/{row['episode']}.txt"
        row["trace_sha256"] = hashlib.sha256((ROOT / row["trace"]).read_bytes()).hexdigest()
    rows.sort(key=lambda r: (r["episode"], r["seat"]))
    (ROOT / "data/perspectives.json").write_text(json.dumps(rows, indent=2, ensure_ascii=False) + "\n")
    for split in ("train", "validation", "development", "all"):
        chosen = [r for r in rows if split == "all" or r["split"] == split]
        (ROOT / "data" / f"{split}.txt").write_text("".join(
            f"{r['episode']} {r['seat']} {r['rank']} {r['team_id']} {r['submission']} {r['trace']}\n" for r in chosen))


if __name__ == "__main__":
    main()
