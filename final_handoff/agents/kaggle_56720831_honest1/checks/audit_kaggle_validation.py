"""Reproduce Kaggle's validation episode of this archive locally: unpack submission archive, load two independent agents as Kaggle does
(main.py exec'd without __file__), replay with the episode's seed and configuration, and require the same final rewards and every
submitted action.  usage: python audit_kaggle_validation.py [replay json]"""
import json
import sys
import tarfile
import tempfile
from pathlib import Path

from kaggle_environments import make
from kaggle_environments.agent import get_last_callable


ROOT = Path(__file__).resolve().parent
ARCHIVE = ROOT / "pavel-bc-opus-v17d-dc12m19-honest-m68.tar.gz"
REPLAY = Path(sys.argv[1]) if len(sys.argv) > 1 else next((ROOT / "validation").glob("episode-*-replay.json"))

remote = json.loads(REPLAY.read_text())
configuration = dict(remote["configuration"])
configuration["seed"] = remote["info"]["seed"]

with tempfile.TemporaryDirectory(prefix="honest1_validation_") as directory:
    agents = []
    for index in range(2):
        agent_dir = Path(directory) / f"agent{index}"
        with tarfile.open(ARCHIVE, "r:gz") as archive:
            archive.extractall(agent_dir, filter="data")
        path = agent_dir / "main.py"
        agents.append(get_last_callable(path.read_text(), path=str(path)))
    local = make("kaggriculture", configuration=configuration, debug=True)
    local.run(agents)

final = local.steps[-1]
assert all(state.status == "DONE" for state in final), [state.status for state in final]
rewards = [state.reward for state in final]
mismatches = [(step, player, local.steps[step][player].action, remote["steps"][step][player]["action"])
              for step in range(1, len(remote["steps"])) for player in range(2)
              if local.steps[step][player].action != remote["steps"][step][player]["action"]]
print(f"seed={configuration['seed']} local rewards={rewards} kaggle rewards={remote['rewards']} "
      f"actions={2 * (len(remote['steps']) - 1)} mismatches={len(mismatches)}")
if mismatches:
    print("first mismatch:", mismatches[0])
assert rewards == remote["rewards"] and not mismatches
