"""Frozen Python source is used only on exogenous observations, never local games."""
import importlib.util
import json
import subprocess
from collections import Counter
from copy import deepcopy
from pathlib import Path

from verify_public_router import pack

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
DONOR = EXP / "research/notebooks/boatlee/v29-r1-adaptive-market-hysteresis"


def main():
    spec = importlib.util.spec_from_file_location("boatlee_v29_reference", DONOR / "extracted_main.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    paths = sorted((EXP / "replays").glob("*replay.json"))[:4]
    replays = [json.loads(path.read_text()) for path in paths]
    fixture = EXP / "tests/boatlee_v29_cases.txt"
    counts = Counter()
    with fixture.open("w") as out:
        def write(obs, step):
            action = module.agent(obs)
            state = module._AM_STATE[obs["player"]]
            counts["mirror"] += state["near_mirror"] is True
            counts["nonmirror"] += state["near_mirror"] is False
            counts["weed_pending"] += bool(module._WEED_STATE[obs["player"]].get("active"))
            counts["pressure"] += any(v >= module._AM_CONFIG["pressure_trigger"] for v in state["pressure"].values())
            counts["extra_budget_used"] += bool(sum(state["added"].values()))
            out.write(pack(obs, action, step == 0, full_tiles=True))

        for replay in replays:
            for seat in range(2):
                for step, pair in enumerate(replay["steps"][:719]):
                    write(dict(pair[seat]["observation"], step=step), step)
        for branch in range(8):
            for step, pair in enumerate(replays[branch % 4]["steps"][:719]):
                obs = deepcopy(pair[0]["observation"])
                obs["step"] = step
                if branch % 2 == 0:
                    obs["farms"][1] = deepcopy(obs["farms"][0])
                else:
                    obs["farms"][1]["money"] = obs["farms"][0]["money"] + 1000
                if branch >= 2:
                    for product in ["STRAWBERRY", "MILK", "WOOL"]:
                        obs["private"]["shed"][product] = 8 if branch < 4 else 30
                        obs["market"]["prices"][product] = 20 if branch in (2, 3) else 300
                        obs["market"]["inventory"][product] = 9900 + (step % 4) * 10
                    obs["town"]["unlocked_shops"] = ["YARN_STORE", "SMOOTHIE_SHOP", "BRUNCH_SPOT"]
                if branch >= 6 and 240 <= step < 312:
                    own = obs["farms"][0]
                    for x, y in [own["farmer"], *own["hands"]]:
                        own["tiles"][y][x] = {"kind": "WEED"}
                write(obs, step)
    binary = EXP / "build/boatlee_v29_parity"
    command = ["conda", "run", "-n", "kaggriculture", "g++", "-std=c++20", "-O2", "-DVERIFY_BOATLEE_V29",
               "-I", str(ROOT), str(EXP / "tests/public_router_parity.cpp"),
               str(EXP / "league/boatlee_v29/source/agent.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    result = subprocess.run(["conda", "run", "-n", "kaggriculture", str(binary), str(fixture)], check=True, capture_output=True, text=True)
    report = {"result": result.stdout.strip(), "coverage": counts, "command": command,
              "scope": "Eight recorded streams plus eight forced observation streams; source-only actions, no Python local gameplay."}
    (EXP / "results/boatlee_v29_parity.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
