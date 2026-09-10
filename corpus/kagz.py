"""Kaggriculture replay corpus codec.

An episode is fully determined by (configuration, seed, action stream): the
recorded actions replayed through the pinned engine regenerate every state
bit-exactly. We therefore store the action stream and treat the engine as the
decompressor. Correctness of the codec is exactly engine parity.

    kagz.py pack   replays/**/ *.json  -o corpus/data     # ingest + certify
    kagz.py verify corpus/data/*.kagz                     # Python re-replay
    kagz.py unpack corpus/data/X.kagz  -o out.json        # byte-lossless
    kagz.py trace  corpus/data/X.kagz  -o X.txt           # fast_game_engine trace
    kagz.py ls     corpus/data                            # manifest table

`pack` proves two things per episode and refuses to write otherwise:
  1. the action stream replayed through the official engine reproduces every
     recorded observation (order-normalized comparison), and
  2. the final money equals the recorded rewards.
It stores the canonical FNV-1a parity hash of all 720 live states, which is the
same digest `fast_game_engine/validate` checks, so the certificate carries over
to the C++ side unchanged.
"""
import argparse
import contextlib
import hashlib
import inspect
import io
import json
import os
import sys
from importlib.metadata import version
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "fast_game_engine"))
import export_trace as ET                                    # noqa: E402
import kagz_format as F                                      # noqa: E402

ENGINE_VERSION = ET.ENGINE_VERSION
OBS_KEYS = ("day", "hour", "step", "farms", "market", "town")


def check_engine():
    import kaggle_environments.envs.kaggriculture.kaggriculture as engine
    installed = version("kaggle-environments")
    if installed != ENGINE_VERSION:
        raise RuntimeError(f"expected kaggle-environments {ENGINE_VERSION}, found {installed}")
    sha = hashlib.sha256(inspect.getsource(engine).encode()).hexdigest()
    if sha != ET.ENGINE_SOURCE_HASH:
        raise RuntimeError(f"unexpected {ENGINE_VERSION} engine source hash {sha}")
    return installed, sha


def _norm(obs_like, private_like):
    """Order-insensitive snapshot. Replay JSON is written with sort_keys, which
    destroys the insertion order of per-unit inventory dicts, so the live and
    recorded states can only be compared as sorted structures."""
    core = {k: obs_like[k] for k in OBS_KEYS}
    return json.dumps([core, private_like], sort_keys=True, default=str)


def _live_snapshot(state):
    return _norm({k: state[0].observation[k] for k in OBS_KEYS},
                 [state[p].observation["private"] for p in range(len(state))])


def _recorded_snapshot(frame):
    return _norm({k: frame[0]["observation"][k] for k in OBS_KEYS},
                 [frame[p]["observation"]["private"] for p in range(len(frame))])


def _chain(hashes):
    h = ET.FNV_OFFSET
    for v in hashes:
        for shift in range(0, 64, 8):
            h ^= (v >> shift) & 0xFF
            h = h * ET.FNV_PRIME & ((1 << 64) - 1)
    return h


def pack_one(src, out_dir, force=False):
    from kaggle_environments import make
    _, engine_sha = check_engine()

    with open(src) as f:
        doc = json.load(f)
    steps = doc["steps"]
    n_steps = len(steps)
    n_seats = len(steps[0])
    seed = doc["info"]["seed"]
    episode_id = int(doc["info"].get("EpisodeId") or Path(src).stem)
    dst = Path(out_dir) / f"{episode_id}.kagz"
    if dst.exists() and not force:
        return dst, "exists", None

    if doc["module_version"] != ENGINE_VERSION:
        raise RuntimeError(f"{src}: replay engine {doc['module_version']} != {ENGINE_VERSION}")
    if seed is None:
        raise RuntimeError(f"{src}: no info.seed; episode is not reproducible")

    # steps[t]['action'] is recorded on the state it PRODUCED, so replaying
    # from steps[t] uses steps[t+1]'s action. Index 0 holds a placeholder.
    actions = [[frame[p].get("action") or {} for p in range(n_seats)] for frame in steps]

    # Wall-clock budget the agent saw. The engine never reads it, so it plays no
    # part in reproduction, but it is in the recorded observation and it is the
    # only direct measure of how much compute each agent spent per turn.
    overage = [[frame[p]["observation"].get("remainingOverageTime")
                for p in range(n_seats)] for frame in steps]

    cfg = dict(doc["configuration"])
    cfg.pop("seed", None)
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        env = make("kaggriculture", configuration=cfg, debug=False)
        env.info = dict(env.info or {})
        env.info["seed"] = seed
        env.reset(num_agents=n_seats)

        hashes = []
        for t in range(n_steps):
            if _live_snapshot(env.state) != _recorded_snapshot(steps[t]):
                raise RuntimeError(f"{src}: state divergence at step {t}")
            hashes.append(ET.parity_hash(ET.canonical_values(t, env.state)))
            if t + 1 < n_steps:
                env.step([actions[t + 1][p] for p in range(n_seats)])

    final = [float(env.state[0].observation["farms"][p]["money"]) for p in range(n_seats)]
    if final != [float(v) for v in doc["rewards"]]:
        raise RuntimeError(f"{src}: final money {final} != recorded rewards {doc['rewards']}")

    Path(out_dir).mkdir(parents=True, exist_ok=True)
    size = F.write(
        dst, episode_id=episode_id, seed=seed, config=doc["configuration"],
        teams=doc["info"]["TeamNames"], statuses=doc["statuses"], rewards=doc["rewards"],
        actions=actions, engine_version=ENGINE_VERSION, engine_sha256=engine_sha,
        anchors=list(enumerate(hashes)), chain_hash=_chain(hashes),
        overage=overage,
        extra=json.dumps({"source": os.path.basename(src),
                          "configuration": doc["configuration"],
                          "source_sha256": hashlib.sha256(open(src, 'rb').read()).hexdigest(),
                          "folder": os.path.basename(os.path.dirname(src)),
                          "schema_version": doc.get("schema_version"),
                          "name": doc.get("name"), "title": doc.get("title"),
                          "version": doc.get("version"),
                          "description": doc.get("description"),
                          "specification": doc.get("specification"),
                          "id": doc.get("id"), "info": doc["info"]}, sort_keys=True))
    return dst, "packed", (os.path.getsize(src), size)


def rebuild_json(ep):
    """Reconstruct the original Kaggle replay document from a .kagz."""
    extra = json.loads(ep["extra"])
    doc = {
        "configuration": extra["configuration"], "description": extra["description"],
        "id": extra["id"], "info": extra["info"], "module_version": ep["engine_version"],
        "name": extra["name"], "rewards": ep["rewards"],
        "schema_version": extra["schema_version"], "specification": extra["specification"],
        "statuses": ep["statuses"], "steps": None, "title": extra["title"],
        "version": extra["version"],
    }
    return doc, extra


def cmd_pack(args):
    srcs = []
    for pat in args.inputs:
        p = Path(pat)
        srcs.extend(sorted(p.rglob("*.json")) if p.is_dir() else [p])
    total_src = total_dst = 0
    packed = failed = skipped = 0
    for s in srcs:
        try:
            dst, status, sizes = pack_one(str(s), args.out, args.force)
        except Exception as e:                                  # noqa: BLE001
            failed += 1
            print(f"FAIL  {s}: {e}")
            continue
        if status == "exists":
            skipped += 1
            print(f"skip  {dst.name} (exists)")
            continue
        packed += 1
        total_src += sizes[0]
        total_dst += sizes[1]
        print(f"ok    {dst.name}  {sizes[0]/1e6:7.2f} MB -> {sizes[1]/1e3:7.1f} KB "
              f"({sizes[0]/sizes[1]:6.0f}x)")
    if packed:
        print(f"\npacked {packed}, skipped {skipped}, failed {failed}   "
              f"{total_src/1e6:.1f} MB -> {total_dst/1e6:.3f} MB "
              f"({total_src/max(total_dst,1):.0f}x)")
    return 1 if failed else 0


def replay(ep, want_obs=False):
    """Re-run an episode from its stored action stream through the official
    engine. Yields (t, live_hash, env) so callers can collect whatever they need
    without each of them re-implementing the off-by-one action indexing."""
    from kaggle_environments import make
    extra = json.loads(ep["extra"])
    cfg = dict(extra["configuration"])
    cfg.pop("seed", None)
    acts = ep["actions"]
    n_seats = len(acts[0])
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        env = make("kaggriculture", configuration=cfg, debug=False)
        env.info = dict(env.info or {})
        env.info["seed"] = ep["seed"]
        env.reset(num_agents=n_seats)
        for t in range(ep["n_steps"]):
            yield t, ET.parity_hash(ET.canonical_values(t, env.state)), env
            if t + 1 < ep["n_steps"]:
                env.step([acts[t + 1][p] for p in range(n_seats)])


def rebuild_steps(ep):
    """Regenerate the full `steps` array of the original replay document."""
    acts = ep["actions"]
    over = ep["overage"]
    n_seats = len(acts[0])
    steps = []
    for t, _, env in replay(ep):
        frame = []
        for p in range(n_seats):
            st = env.state[p]
            obs = json.loads(json.dumps(st.observation, default=dict))
            if over is not None and over[t][p] is not None:
                obs["remainingOverageTime"] = over[t][p]
            frame.append({"action": acts[t][p], "info": {}, "observation": obs,
                          "reward": st.reward, "status": st.status})
        steps.append(frame)
    return steps


def cmd_verify(args):
    check_engine()
    bad = 0
    for path in _kagz_paths(args.inputs):
        ep = F.read(path)
        stored = [h for _, h in ep["anchors"]]
        live = []
        env = None
        for _, h, env in replay(ep):
            live.append(h)
        n_seats = len(ep["actions"][0])
        money = [float(env.state[0].observation["farms"][p]["money"]) for p in range(n_seats)]
        mism = [i for i, (a, b) in enumerate(zip(stored, live)) if a != b]
        ok = (not mism and len(stored) == len(live)
              and _chain(live) == ep["chain_hash"]
              and money == [float(v) for v in ep["rewards"]])
        detail = ""
        if args.roundtrip:
            src = Path(args.replays) / json.loads(ep["extra"])["folder"] / json.loads(ep["extra"])["source"]
            doc, extra = rebuild_json(ep)
            doc["steps"] = rebuild_steps(ep)
            with open(src) as f:
                original = json.load(f)
            same = doc == original
            ok = ok and same
            detail = f", roundtrip {'exact' if same else 'DIFFERS'}"
        bad += 0 if ok else 1
        print(f"{'PASS' if ok else 'FAIL'}  {Path(path).name}  {len(live)} states, "
              f"{len(mism)} mismatches, money {money} vs rewards {ep['rewards']}{detail}")
        if mism:
            print(f"      first mismatching steps: {mism[:5]}")
    return 1 if bad else 0


def cmd_unpack(args):
    ep = F.read(args.input)
    doc, _ = rebuild_json(ep)
    check_engine()
    doc["steps"] = rebuild_steps(ep)
    out = args.out or (Path(args.input).with_suffix(".json"))
    with open(out, "w") as f:
        json.dump(doc, f, sort_keys=True, separators=(", ", ": "))
    print(f"wrote {out}")
    return 0


def cmd_trace(args):
    """Emit the fast_game_engine trace format so build/validate, build/bench and
    python_bench.py accept a downloaded replay unchanged.

    money and market inventory are regenerated here by the official Python
    engine rather than read back from the container, so the TRUTH block stays an
    independent witness; the stored parity hashes are asserted against it.
    """
    check_engine()
    for path in _kagz_paths(args.inputs):
        ep = F.read(path)
        acts = ep["actions"]
        n_seats = len(acts[0])
        money, inv, hashes = [], [], []
        for _, h, env in replay(ep):
            obs = env.state[0].observation
            money.append([float(obs["farms"][p]["money"]) for p in range(n_seats)])
            inv.append([int(v) for v in obs["market"]["inventory"].values()])
            hashes.append(h)
        stored = [h for _, h in ep["anchors"]]
        if hashes != stored:
            raise RuntimeError(f"{path}: regenerated hashes disagree with the container")

        cfg = json.loads(ep["extra"])["configuration"]
        conf = [cfg["episodeSteps"], cfg["boardSize"], cfg["startingMoney"],
                cfg["maxMarketOrdersPerTurn"], cfg["turnsPerDay"], cfg["shedCapacity"],
                cfg["weedSpawnChance"], cfg["townShopUnlockInterval"],
                cfg["townShopSellInterval"], cfg["townCenterSellInterval"],
                cfg["farmHandCostMult"]]
        n = ep["n_steps"] - 1
        lines = [f"{ep['seed']} {n}", "CONFIG " + " ".join(str(v) for v in conf),
                 f"ENGINE {ep['engine_version']} {ep['engine_sha256']}"]
        for t in range(n):
            for seat in range(n_seats):
                act = acts[t + 1][seat]
                units = [act.get("farmer") or ["PASS"]] + list(act.get("hands") or [])
                orders = list(act.get("market") or [])
                parts = [str(len(units)), str(len(orders))]
                for u in units:
                    parts += [str(v) for v in ET.enc_unit(u)]
                for o in orders:
                    parts += [str(v) for v in ET.enc_order(o)]
                lines.append(" ".join(parts))
        lines.append("TRUTH")
        for i in range(len(money)):
            lines.append(" ".join([f"{money[i][0]:.0f}", f"{money[i][1]:.0f}"]
                                  + [str(v) for v in inv[i]] + [str(hashes[i])]))
        out = Path(args.out or ".") / f"replay_kagz_{ep['episode_id']}.txt"
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_text("\n".join(lines) + "\n")
        print(f"wrote {out}  ({out.stat().st_size/1e3:.1f} KB)")
    return 0


def cmd_ls(args):
    rows = []
    for path in _kagz_paths(args.inputs):
        ep = F.read(path)
        extra = json.loads(ep["extra"])
        rows.append((ep["episode_id"], ep["seed"], ep["teams"], ep["rewards"],
                     os.path.getsize(path), extra.get("folder", "")))
    rows.sort()
    print(f"{'episode':>10} {'seed':>11} {'KB':>6}  {'reward0':>8} {'reward1':>8}  teams")
    for eid, seed, teams, rew, size, folder in rows:
        print(f"{eid:>10} {seed:>11} {size/1e3:6.1f}  {rew[0]:>8.0f} {rew[1]:>8.0f}  "
              f"{teams[0]} vs {teams[1]}")
    print(f"\n{len(rows)} episodes, {sum(r[4] for r in rows)/1e6:.3f} MB total")
    return 0


def _kagz_paths(inputs):
    out = []
    for pat in inputs:
        p = Path(pat)
        out.extend(sorted(p.glob("*.kagz")) if p.is_dir() else [p])
    return [str(x) for x in out]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("pack"); p.add_argument("inputs", nargs="+")
    p.add_argument("-o", "--out", default="data"); p.add_argument("-f", "--force", action="store_true")
    p.set_defaults(fn=cmd_pack)
    p = sub.add_parser("verify"); p.add_argument("inputs", nargs="+")
    p.add_argument("--roundtrip", action="store_true",
                   help="also rebuild the original replay JSON and compare it exactly")
    p.add_argument("--replays", default="../replays", help="root of the source replay tree")
    p.set_defaults(fn=cmd_verify)
    p = sub.add_parser("trace"); p.add_argument("inputs", nargs="+")
    p.add_argument("-o", "--out", default="traces"); p.set_defaults(fn=cmd_trace)
    p = sub.add_parser("unpack"); p.add_argument("input"); p.add_argument("-o", "--out")
    p.set_defaults(fn=cmd_unpack)
    p = sub.add_parser("ls"); p.add_argument("inputs", nargs="*", default=["data"])
    p.set_defaults(fn=cmd_ls)
    args = ap.parse_args()
    sys.exit(args.fn(args))


if __name__ == "__main__":
    main()
