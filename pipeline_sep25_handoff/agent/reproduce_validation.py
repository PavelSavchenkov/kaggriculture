"""Replays Kaggle's validation episode of a submission (self-play, both seats this agent) locally with
an agent folder and requires equal final rewards and every submitted action. Download the replay from
the submission's episode list (episode-<id>-replay.json).

    python reproduce_validation.py <agent folder> <episode-<id>-replay.json>
"""
import json
import sys
from pathlib import Path

from kaggle_environments import make
from kaggle_environments.agent import get_last_callable


def load(agent_dir):
    path = Path(agent_dir) / "main.py"
    return get_last_callable(path.read_text(), path=str(path))


agent_dir, replay_path = sys.argv[1], sys.argv[2]
replay = json.loads(Path(replay_path).read_text())
seed = replay["info"]["seed"]
environment = make("kaggriculture", configuration={"seed": seed}, debug=True)
environment.run([load(agent_dir), load(agent_dir)])

steps = [[json.dumps(state.action, sort_keys=True) for state in step] for step in environment.steps]
expected = [[json.dumps(state["action"], sort_keys=True) for state in step] for step in replay["steps"]]
different = [(index, seat) for index, (a, b) in enumerate(zip(steps, expected)) for seat in range(2) if a[seat] != b[seat]]
rewards = [state.reward for state in environment.steps[-1]]
print(json.dumps({"episode": replay["id"], "seed": seed, "steps": [len(steps), len(expected)], "rewards": rewards,
                  "kaggle_rewards": replay["rewards"], "different_actions": len(different)}))
if len(steps) != len(expected) or different or rewards != replay["rewards"]:
    raise SystemExit(f"validation episode not reproduced; first differences {different[:5]}")
