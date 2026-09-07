"""Exercise the replay diagnostic on deliberate no-op and successful actions."""
import json
from collections import Counter
from copy import deepcopy
from pathlib import Path

from analyze_replays import apply_units

EXP = Path(__file__).resolve().parents[1]


def main():
    replay = json.loads((EXP / "replays/episode-106328416-replay.json").read_text())
    observations = replay["steps"][0]
    checks = []
    for command, expected in ((["HARVEST"], 1), (["FEED"], 1), (["PLANT", "WHEAT"], 1), (["NORTH"], 0), (["PASS"], 0)):
        farms = deepcopy(observations[0]["observation"]["farms"])
        private = [deepcopy(p["observation"]["private"]) for p in observations]
        private[0]["seeds"]["WHEAT"] = 0
        context = {"unit_requests": 0, "unit_faults": Counter()}
        apply_units(farms, private, [{"farmer": command}, {"farmer": ["PASS"]}], 0, 1, {0: context}, [])
        actual = sum(context["unit_faults"].values())
        assert actual == expected, (command, actual, expected)
        checks.append({"command": command, "faults": actual})
    (EXP / "results/replay_fault_counter_checks.json").write_text(json.dumps(checks, indent=2) + "\n")
    print(f"{len(checks)} deliberate diagnostic cases pass")


if __name__ == "__main__":
    main()
