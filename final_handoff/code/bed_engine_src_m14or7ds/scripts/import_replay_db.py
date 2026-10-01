"""Imports the public replay database xishengfeng/kaggriculture-replay-db (data/replay_db/
kaggriculture-replay-db.zip: per episode the seed, config, both seats' actions and daily
money/market snapshots) as engine traces.

Selects public 720-step episodes with both seats DONE that are not already in data/traces,
writes action-only inputs (data/replay_db/partial) for build/db_trace, which re-simulates each
episode and keeps it only if every daily snapshot matches, and records both seats in
data/replay_db/perspectives.csv (episode, seat, team, team_name, submission, date, score,
reward, trace). Run db_trace on data/replay_db/list_*.txt afterwards.

usage: import_replay_db.py [--chunks 16] [--part k --parts n]   (parts: disjoint shard subsets; resumable)
"""
import argparse
import csv
import io
import json
import sys
import zipfile
from pathlib import Path

import pyarrow.parquet as pq
import zstandard

ROOT = Path(__file__).resolve().parents[1]
REPO = ROOT.parents[2]
sys.path.insert(0, str(REPO / "fast_game_engine"))
from export_trace import ITEMS, ENGINE_VERSION, ENGINE_SOURCE_HASH, enc_unit, enc_order  # noqa: E402

CONFIG_KEYS = ("episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn", "turnsPerDay",
               "shedCapacity", "weedSpawnChance", "townShopUnlockInterval", "townShopSellInterval",
               "townCenterSellInterval", "farmHandCostMult")
DB = ROOT / "data/replay_db"


def action_words(action, max_orders):
    action = action or {}
    units = [action.get("farmer") or ["PASS"], *(action.get("hands") or [])]
    orders = (action.get("market") or [])[:max_orders]
    words = [len(units), len(orders)]
    for unit in units:
        words.extend(enc_unit(unit))
    for order in orders:
        words.extend(enc_order(order))
    return " ".join(map(str, words))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--chunks", type=int, default=16)
    parser.add_argument("--part", type=int, default=0)
    parser.add_argument("--parts", type=int, default=1)
    args = parser.parse_args()
    have = {int(p.stem) for p in (ROOT / "data/traces").glob("*.txt") if p.stem.isdigit()}
    (DB / "partial").mkdir(exist_ok=True)
    out_dir = ROOT / "data/traces_db"
    out_dir.mkdir(exist_ok=True)
    teams = {}
    unzip = zstandard.ZstdDecompressor()
    jobs, rows = [], []
    counts = {"rows": 0, "known": 0, "not_public": 0, "not_done": 0, "bad_length": 0, "written": 0}
    with zipfile.ZipFile(DB / "kaggriculture-replay-db.zip") as archive:
        for r in pq.read_table(io.BytesIO(archive.read("replay_db/index/teams.parquet"))).to_pylist():
            teams[r["team_id"]] = r["team_name"]
        shards = sorted(n for n in archive.namelist() if n.startswith("replay_db/shards/") and n.endswith(".parquet"))
        shards = shards[args.part::args.parts]
        for shard in shards:
            for r in pq.read_table(io.BytesIO(archive.read(shard))).to_pylist():
                counts["rows"] += 1
                episode = r["episode_id"]
                if episode in have:
                    counts["known"] += 1
                    continue
                if r["n_steps"] != 720 or r["first_bad_step"] != -1:
                    counts["bad_length"] += 1
                    continue
                if json.loads(r["final_status"]) != ["DONE", "DONE"]:
                    counts["not_done"] += 1
                    continue
                partial, trace = DB / "partial" / f"{episode}.txt", out_dir / f"{episode}.txt"
                if not partial.exists():
                    config = json.loads(r["config"])
                    actions = json.loads(unzip.decompress(r["actions_zstd"], max_output_size=1 << 30))
                    if len(actions) == 720:  # the first entry precedes the first transition
                        actions = actions[1:]
                    if len(actions) != 719 or any(len(turn) != 2 for turn in actions):
                        counts["bad_length"] += 1
                        continue
                    daily = json.loads(unzip.decompress(r["daily_zstd"], max_output_size=1 << 28))
                    lines = [f"{r['seed']} 719", "CONFIG " + " ".join(str(config[k]) for k in CONFIG_KEYS),
                             f"ENGINE {ENGINE_VERSION} {ENGINE_SOURCE_HASH}"]
                    for turn in actions:
                        lines.extend(action_words(a, config["maxMarketOrdersPerTurn"]) for a in turn)
                    lines.append(f"DAILY {len(daily)}")
                    for d in daily:
                        lines.append(" ".join(map(str, [d["step"], *d["money"], *(d["inventory"][p] for p in ITEMS[:9])])))
                    partial.write_text("\n".join(lines) + "\n")
                jobs.append(f"{partial} {trace}")
                counts["written"] += 1
                for seat in (0, 1):
                    team = r[f"team_id_{seat}"]
                    rows.append([episode, seat, team, teams.get(team, ""), r[f"submission_id_{seat}"], r["create_time"][:19],
                                 r[f"updated_score_{seat}"], r[f"reward_{seat}"], str(trace.relative_to(ROOT))])
    suffix = f"_{args.part}" if args.parts > 1 else ""
    with (DB / f"perspectives{suffix}.csv").open("w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["episode", "seat", "team", "team_name", "submission", "time", "score", "reward", "trace"])
        w.writerows(rows)
    for k in range(args.chunks):
        (DB / f"list{suffix}_{k:02d}.txt").write_text("".join(j + "\n" for j in jobs[k::args.chunks]))
    print(counts)


if __name__ == "__main__":
    main()
