"""Unpack submission.tar.gz and play full official-environment games with main.py loaded the way
Kaggle loads it (kaggle_environments exec()s it without __file__). Fails on any non-DONE status.

    python verify_submission.py [--archive A] [--output O] [--library L] [--quick]

--library swaps another build of libopus_lb_bridge.so into the unpacked agent (kernel rebuild).
"""

import argparse
import hashlib
import json
import platform
import shutil
import tarfile
import tempfile
import time
from pathlib import Path

import kaggle_environments
from kaggle_environments import make
from kaggle_environments.agent import get_last_callable


ROOT = Path(__file__).resolve().parent
# The archive's files as built by build_submission.py (BUILD.json, travels with this script), plus their folders.
BUILT = [name for name in json.loads((ROOT / "BUILD.json").read_text()) if name != "submission.tar.gz"]
EXPECTED = set(BUILT) | {str(Path(name).parent) for name in BUILT if "/" in name}
# (name, opponent, seat of this agent, seed); "self" is self-play, as in Kaggle's validation episode.
GAMES = [("self_seed1", "self", 0, 1), ("self_seed1_repeat", "self", 0, 1),
         ("starter_seat0", "starter", 0, 2), ("starter_seat1", "starter", 1, 3)]


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load(agent_dir):
    """A fresh agent instance, loaded as kaggle_environments.agent.build_agent does, with timing."""
    path = agent_dir / "main.py"
    function = get_last_callable(path.read_text(), path=str(path))
    times = []

    def timed(observation, configuration):
        start = time.perf_counter()
        action = function(observation, configuration)
        times.append(time.perf_counter() - start)
        return action

    return timed, times


def play(agent_dir, opponent, seat, seed):
    agents, times = [], {}
    for index in range(2):
        if index == seat or opponent == "self":
            function, times[index] = load(agent_dir)
            agents.append(function)
        else:
            agents.append(opponent)
    environment = make("kaggriculture", configuration={"seed": seed}, debug=True)
    start = time.monotonic()
    environment.run(agents)
    final = environment.steps[-1]
    return {
        "opponent": opponent, "seat": seat, "seed": seed,
        "steps": len(environment.steps),
        "statuses": [str(state.status) for state in final],
        "rewards": [state.reward for state in final],
        "remaining_overage": [state.observation.get("remainingOverageTime") for state in final],
        "calls": {index: len(t) for index, t in times.items()},
        "max_call_seconds": {index: max(t) for index, t in times.items()},
        "calls_over_1s": {index: sum(x > 1 for x in t) for index, t in times.items()},
        "agent_seconds": {index: sum(t) for index, t in times.items()},
        "game_seconds": time.monotonic() - start,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", type=Path, default=ROOT / "submission.tar.gz")
    parser.add_argument("--output", type=Path, default=ROOT / "VERIFICATION.json")
    parser.add_argument("--library", type=Path)
    parser.add_argument("--quick", action="store_true", help="only the first self-play game")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory(prefix="bc_opus_submission_") as directory:
        agent_dir = Path(directory) / "agent"
        with tarfile.open(args.archive, "r:gz") as archive:
            names = {member.name for member in archive.getmembers()}
            if names != EXPECTED:
                raise RuntimeError(f"archive members {sorted(names)} differ from {sorted(EXPECTED)}")
            archive.extractall(agent_dir, filter="data")
        if args.library:
            shutil.copy2(args.library, agent_dir / "libopus_lb_bridge.so")
        report = {
            "platform": platform.platform(), "libc": platform.libc_ver(),
            "kaggle_environments": kaggle_environments.__version__,
            "archive_sha256": sha256(args.archive),
            "library_sha256": sha256(agent_dir / "libopus_lb_bridge.so"),
            "main_sha256": sha256(agent_dir / "main.py"),
            "games": {},
        }
        for name, opponent, seat, seed in GAMES[:1] if args.quick else GAMES:
            result = play(agent_dir, opponent, seat, seed)
            report["games"][name] = result
            print(name, json.dumps(result), flush=True)
            if result["statuses"] != ["DONE", "DONE"]:
                raise RuntimeError(f"{name}: statuses {result['statuses']}")
    args.output.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
