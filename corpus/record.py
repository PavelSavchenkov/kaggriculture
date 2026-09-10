"""Record a live agent's play as a .kagz episode.

The corpus was built from downloaded Kaggle replays, but nothing about the
format is specific to them: an episode is (config, seed, action stream), and a
live agent produces exactly that. Recording an incumbent's own play makes the
whole mining stack - day contracts, edits, solver recompiles, splices - apply to
it, rather than only to donors.

    record.py <lb_root> <agent_a> <agent_b> <seed> <out.kagz>
"""
from __future__ import annotations

import os
for _v in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_v, "1")

import contextlib, hashlib, io, json, sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "fast_game_engine"))
import export_trace as ET                                     # noqa: E402
import kagz_format as F                                       # noqa: E402


def record(lb_root: Path, a_id: str, b_id: str, seed: int, out: Path) -> dict:
    sys.path.insert(0, str(lb_root / "src"))
    from lb.config import load_config
    from lb.match import load_agent
    import kaggle_environments.envs.kaggriculture.kaggriculture as engine
    from kaggle_environments import make

    cwd = Path.cwd()
    os.chdir(lb_root)
    try:
        cfg = load_config()
        agents = [load_agent("agents", a_id, cfg), load_agent("agents", b_id, cfg)]
    finally:
        os.chdir(cwd)

    steps = 720
    actions = [[{"farmer": ["PASS"], "hands": [], "market": []} for _ in range(2)]]
    hashes = []
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        env = make("kaggriculture", configuration={"episodeSteps": steps, "seed": int(seed)},
                   debug=False)
        env.reset(num_agents=2)
        state = env.state
        for index in range(steps):
            hashes.append(ET.parity_hash(ET.canonical_values(index, state)))
            if index + 1 >= steps:
                break
            chosen = []
            for seat in (0, 1):
                act = agents[seat](state[seat].observation, env.configuration)
                state[seat].action = act
                chosen.append(json.loads(json.dumps(act)))
            actions.append(chosen)
            state = engine.interpreter(state, env)
            for seat in (0, 1):
                state[seat].observation.step = index + 1
        farms = state[0].observation["farms"]
        rewards = [float(farms[0]["money"]), float(farms[1]["money"])]

    chain = ET.FNV_OFFSET
    for v in hashes:
        for shift in range(0, 64, 8):
            chain ^= (v >> shift) & 0xFF
            chain = chain * ET.FNV_PRIME & ((1 << 64) - 1)

    config = dict(env.configuration)
    config["seed"] = None
    episode_id = int(hashlib.sha256(f"{a_id}|{b_id}|{seed}".encode()).hexdigest()[:12], 16)
    F.write(out, episode_id=episode_id, seed=int(seed), config=config,
            teams=[a_id, b_id], statuses=["DONE", "DONE"], rewards=rewards,
            actions=actions, engine_version=ET.ENGINE_VERSION,
            engine_sha256=ET.ENGINE_SOURCE_HASH,
            anchors=list(enumerate(hashes)), chain_hash=chain,
            extra=json.dumps({"source": f"recorded:{a_id}", "configuration": config,
                              "folder": "recorded", "schema_version": 1,
                              "name": "kaggriculture", "title": "Kaggriculture",
                              "version": "0.1.0", "description": "", "specification": {},
                              "id": episode_id,
                              "info": {"TeamNames": [a_id, b_id], "seed": int(seed),
                                       "EpisodeId": episode_id}}, sort_keys=True))
    return {"episode_id": episode_id, "rewards": rewards, "bytes": out.stat().st_size}


if __name__ == "__main__":
    lb = Path(sys.argv[1]); a, b, seed, out = sys.argv[2], sys.argv[3], int(sys.argv[4]), Path(sys.argv[5])
    info = record(lb, a, b, seed, out)
    print(f"recorded {a} vs {b} seed {seed}: rewards {info['rewards']}, "
          f"{info['bytes']/1000:.1f} KB -> {out}")
