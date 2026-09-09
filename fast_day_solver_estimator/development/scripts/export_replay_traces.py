"""Convert downloaded development replays into engine parity traces, offline only."""
import argparse
import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path
from types import SimpleNamespace


EXP = Path(__file__).resolve().parents[1]
REPO = EXP.parents[2]
sys.path.insert(0, str(REPO))
from fast_game_engine.export_trace import ENGINE_SOURCE_HASH, ENGINE_VERSION, ITEMS, canonical_values, enc_order, enc_unit, parity_hash


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("intake", type=Path)
    parser.add_argument("--limit", type=int)
    args = parser.parse_args()
    intake = args.intake.resolve()
    manifest = json.loads((intake / "DOWNLOAD.json").read_text())
    output = EXP / "data" / f"{intake.name}_traces_v1"
    output.mkdir(exist_ok=False)
    games = {}
    for team in manifest["teams"]:
        for game in team["episodes"]:
            games.setdefault(game["episode_id"], {"game": game, "teams": []})["teams"].append(team)
    rows = []
    for episode, entry in sorted(games.items())[:args.limit]:
        raw_path = intake / entry["game"]["file"]
        r = json.loads(raw_path.read_text())
        seed = r["info"]["seed"]
        frames = r["steps"]
        if len(frames) != 720:
            rows.append({"episode": episode, "status": "unsupported_horizon", "frames": len(frames)})
            continue
        cfg = r["configuration"]
        conf = [cfg[k] for k in ["episodeSteps", "boardSize", "startingMoney", "maxMarketOrdersPerTurn",
                "turnsPerDay", "shedCapacity", "weedSpawnChance", "townShopUnlockInterval", "townShopSellInterval",
                "townCenterSellInterval", "farmHandCostMult"]]
        names = r["info"]["TeamNames"]
        seats = []
        for team in entry["teams"]:
            matches = [i for i, name in enumerate(names) if name.strip().casefold() == team["name"].strip().casefold()]
            if len(matches) != 1:
                raise ValueError(f"team identity mismatch {episode}: {team['name']} / {names}")
            seats.append({"seat": matches[0], "team_id": team["team_id"], "team": team["name"]})
        target = output / f"{episode}.txt"
        with target.open("w") as stream:
            stream.write(f"{seed} {len(frames)-1}\nCONFIG " + " ".join(map(str, conf)) + "\n")
            stream.write(f"ENGINE {ENGINE_VERSION} {ENGINE_SOURCE_HASH}\n")
            for frame in frames[1:]:
                for seat in range(2):
                    act = frame[seat]["action"] or {}
                    units = [act.get("farmer") or ["PASS"], *(act.get("hands") or [])]
                    orders = act.get("market") or []
                    parts = [len(units), len(orders)]
                    for unit in units:
                        parts.extend(enc_unit(unit))
                    for order in orders:
                        parts.extend(enc_order(order))
                    stream.write(" ".join(map(str, parts)) + "\n")
            stream.write("TRUTH\n")
            for step, frame in enumerate(frames):
                # The replay API alphabetizes dictionaries and loses carried
                # insertion order. Compare all observed counts in item order;
                # actual C++ execution still retains insertion-order semantics.
                wrapped = []
                for state in frame:
                    observed = dict(state["observation"])
                    market = observed["market"]
                    observed["market"] = {kind: {item: market[kind][item] for item in ITEMS[:9]}
                                           for kind in ["inventory", "prices"]}
                    private = observed["private"]
                    observed["private"] = {"shed": {item: private["shed"][item] for item in ITEMS},
                        "seeds": {item: private["seeds"][item] for item in ITEMS[:5]},
                        "inventories": [{item: inv[item] for item in ITEMS if item in inv} for inv in private["inventories"]]}
                    wrapped.append(SimpleNamespace(status=state["status"], observation=SimpleNamespace(**observed)))
                obs = frame[0]["observation"]
                money = [int(f["money"]) for f in obs["farms"]]
                inventory = [obs["market"]["inventory"][item] for item in ITEMS[:9]]
                state_hash = parity_hash(canonical_values(step, wrapped))
                stream.write(" ".join(map(str, [*money, *inventory, state_hash])) + "\n")
        rows.append({"episode": episode, "status": "exported_unverified", "trace": str(target.relative_to(EXP)),
                     "trace_sha256": hashlib.sha256(target.read_bytes()).hexdigest(), "seats": seats,
                     "replay_sha256": hashlib.sha256(raw_path.read_bytes()).hexdigest()})
        (output / "MANIFEST.json").write_text(json.dumps({"utc": datetime.now(timezone.utc).isoformat(), "games": rows}, indent=2) + "\n")
        print(f"exported {episode}, {len(rows)}/{len(games)}", flush=True)


if __name__ == "__main__":
    main()
