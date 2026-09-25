"""Local-LB games of the bc_opus bridge (as experiments/v10/sep24_BC_opus/scripts/lb_play.py)
that also record both seats' submitted actions as a raw trace (<out>/<opp>_<seed>_seat<k>.raw):
header "seed turns", "CONFIG ...", then one line per seat per turn in the engine text
format, then "MONEY m0 m1" (final money from the Python engine, for the parity check in
finalize_trace).

usage: lb_trace_play.py out_dir opponent [opponent ...] --seeds 700-715 --procs 10
env: BUILD=<abs build dir with libopus_lb_bridge.so>, BC_OPUS_MODEL=<model.bin>
"""
import os
for name in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS", "NUMEXPR_NUM_THREADS"):
    os.environ[name] = "1"
import argparse
import ast
import contextlib
import copy
import io
import json
import multiprocessing
import sys
import traceback
from pathlib import Path

EXP = Path(os.environ["BC_EXPERIMENT"])  # experiment folder made by setup_experiment.sh
sys.path.insert(0, str(EXP / "scripts"))
import lb_play  # noqa: E402  (Native bridge, observation encoding, Local-LB snapshot path)
from export_trace import enc_unit, enc_order  # noqa: E402  (on sys.path via lb_play)


def encode(action):
    action = action or {}
    units = [action.get("farmer") or ["PASS"]] + list(action.get("hands") or [])
    orders = list(action.get("market") or [])
    parts = [str(len(units)), str(len(orders))]
    for u in units:
        parts += [str(v) for v in enc_unit(u)]
    for o in orders:
        parts += [str(v) for v in enc_order(o)]
    return " ".join(parts)


def play(opponent_name, seed, seat, raw_path):
    with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
        from kaggle_environments import make
        import kaggle_environments.envs.kaggriculture.kaggriculture as engine
        env = make("kaggriculture", configuration={"episodeSteps": 720, "seed": seed}, debug=False)
        env.reset(num_agents=2)
        sys.path.insert(0, str(lb_play.LB / "src"))
        from lb.validate import load_agent_callable
        if opponent_name.startswith("extra:"):
            main_py = EXP / "data/extra_opponents" / opponent_name[6:] / "main.py"
            entry = [n.name for n in ast.parse(main_py.read_text()).body if isinstance(n, ast.FunctionDef)][-1]
        else:
            main_py, entry = lb_play.LB / "agents" / opponent_name / "main.py", "agent"
        opponent = load_agent_callable(main_py, module_label=opponent_name.replace(":", "_"), entry_callable=entry)
    policy = lb_play.Native(env.configuration, seat)
    agents = ((lambda o, c: policy.act(o, c)), opponent) if seat == 0 else (opponent, (lambda o, c: policy.act(o, c)))
    lines = []
    state = env.state
    for player in (0, 1):
        state[player].observation.step = 0
    for step in range(1, 720):
        observations = [copy.deepcopy(s.observation) for s in state]
        for player in (0, 1):
            with contextlib.redirect_stdout(io.StringIO()):
                state[player].action = agents[player](observations[player], env.configuration)
        for player in (0, 1):
            lines.append(encode(state[player].action))
        state = engine.interpreter(state, env)
        for player in (0, 1):
            state[player].observation.step = step
        if state[0].status == "DONE":
            break
    money = [float(f["money"]) for f in state[0].observation["farms"]]
    cfg = env.configuration
    conf = [cfg[k] for k in lb_play.CONFIG_KEYS]
    raw_path.write_text("\n".join([f"{seed} {len(lines) // 2}", "CONFIG " + " ".join(str(v) for v in conf), *lines,
                                   f"MONEY {money[0]:.0f} {money[1]:.0f}"]) + "\n")
    return dict(opponent=opponent_name, seed=seed, seat=seat, money=money, margin=money[seat] - money[seat ^ 1],
                statuses=[s.status for s in state], **policy.stats())


def job(args):
    out_dir, opponent, seed, seat = args
    path = out_dir / f"{opponent}_{seed}_seat{seat}.json"
    if path.exists():
        return json.loads(path.read_text())
    try:
        result = play(opponent, seed, seat, out_dir / f"{opponent}_{seed}_seat{seat}.raw")
    except Exception:
        result = dict(opponent=opponent, seed=seed, seat=seat, error=traceback.format_exc())
    path.write_text(json.dumps(result, indent=1))
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("out_dir", type=Path)
    parser.add_argument("opponents", nargs="+")
    parser.add_argument("--seeds", default="700-715")
    parser.add_argument("--procs", type=int, default=10)
    args = parser.parse_args()
    first, last = map(int, args.seeds.split("-"))
    args.out_dir.mkdir(parents=True, exist_ok=True)
    jobs = [(args.out_dir.resolve(), o, s, k) for o in args.opponents for s in range(first, last + 1) for k in (0, 1)]
    results = []
    with multiprocessing.get_context("fork").Pool(args.procs, maxtasksperchild=1) as pool:
        for r in pool.imap_unordered(job, jobs):
            results.append(r)
            if "error" in r:
                print(f"error {r['opponent']} {r['seed']} seat{r['seat']}: {r['error'].splitlines()[-1]}", flush=True)
    for opponent in args.opponents:
        rs = [r for r in results if r["opponent"] == opponent and "error" not in r]
        if rs:
            wins = sum(r["margin"] > 0 for r in rs)
            print(f"{opponent}: games {len(rs)} wins {wins} mean margin {sum(r['margin'] for r in rs) / len(rs):.1f}", flush=True)


if __name__ == "__main__":
    main()
