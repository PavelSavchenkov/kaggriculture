"""Frozen-source differential verification; Python never chooses local-game actions."""
import importlib.util
import json
import subprocess
from collections import Counter
from copy import deepcopy
from pathlib import Path
from verify_public_router import pack

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
DONOR = EXP / "research/refresh_0804/notebooks/yamakawanin/king-v4e-rc4"


def main():
    spec = importlib.util.spec_from_file_location("king_reference", DONOR / "extracted_main.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    paths = sorted((EXP / "replays").glob("*replay.json"))[:4]
    replays = [json.loads(p.read_text()) for p in paths]
    fixture = EXP / "tests/king_cases.txt"
    counts = Counter()
    with fixture.open("w") as out:
        def write(obs, step):
            action = module.agent(obs)
            p = obs["player"]
            counts[module._V3M_MODE[p]] += 1
            counts["YARN_SECOND"] += bool(module._V4E_Y2[p])
            counts["recovery_queued"] += bool(module._V2_QUEUE[p])
            counts["LOW"] += module._Q30V1_STATE[p].get("mode") == "LOW"
            counts["MID"] += module._Q30V1_STATE[p].get("mode") == "MID"
            counts["HIGH"] += module._Q30V1_STATE[p].get("mode") == "HIGH"
            counts["S1_BAL_override"] += module._V3J2_S1_OVERRIDE[p] == "BAL"
            if module._V3M_MODE[p] == "HIGHCAP" and module._V3M_TAIL._A is not None:
                counts[f"tail_route_{module._V3M_TAIL._A.cur}"] += 1
            for hit in module._RC3_HITS[p] + module._RC4_HITS[p]:
                if hit[1] == step:
                    counts[hit[0]] += 1
            out.write(pack(obs, action, step == 0, full_tiles=True))

        for replay in replays:
            for seat in range(2):
                for step, pair in enumerate(replay["steps"][:719]):
                    write(dict(pair[seat]["observation"], step=step), step)
        for branch in range(12):
            for step, pair in enumerate(replays[branch % 4]["steps"][:719]):
                obs = deepcopy(pair[0]["observation"])
                obs["step"] = step
                own, opp = obs["farms"]
                if branch < 4:
                    if step == 72:
                        opp["money"] = 100
                        obs["town"]["unlocked_shops"] = ["BAKERY"]
                    if step == 226:
                        obs["town"]["unlocked_shops"] = ["YARN_STORE" if branch in [1, 2] else "BAKERY"]
                    if step == 360:
                        obs["market"]["prices"]["CARROT"] = 42 if branch == 2 else 41
                    if step == 433:
                        obs["market"]["inventory"]["MILK"] = 10067 if branch == 3 else 10066
                else:
                    if step == 1 and branch not in [6, 7]:
                        obs["market"]["inventory"]["WHEAT"] = 9959 if branch in [8, 9] else 9940
                    if step >= 72:
                        obs["town"]["unlocked_shops"] = ["YARN_STORE" if branch % 2 else "ICE_CREAM_SHOP"]
                        if step >= 144:
                            obs["town"]["unlocked_shops"].append("YARN_STORE" if branch % 2 == 0 else "BAKERY")
                    if step == 72:
                        opp["money"] = 500
                    if branch in [4, 5] and step >= 240:
                        own["money"] = 10000
                        obs["private"]["shed"].update({"SHEEP": 0, "FERTILIZER": 12, "WOOL": 8, "MILK": 8})
                    if branch in [6, 7]:
                        obs["farms"][1] = deepcopy(own)
                        if step == 1:
                            obs["market"]["inventory"]["WHEAT"] = 9969
                    if branch in [8, 9] and 240 <= step < 312:
                        obs["private"]["seeds"] = dict.fromkeys(["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON"], 100)
                        for x, y in [own["farmer"], *own["hands"]]:
                            own["tiles"][y][x] = {"kind": "WEED"}
                    if branch in [10, 11]:
                        if step == 2:
                            opp.update(money=66, farmer=[4, 3], hands=[[4, 4]] * 5, unlocked_quadrants=["NW"])
                        if step % 24 >= 12:
                            own["money"] = 1
                write(obs, step)
    binary = EXP / "build/king_parity"
    command = ["conda", "run", "-n", "kaggriculture", "g++", "-std=c++20", "-O2", "-DVERIFY_KING",
               "-I", str(ROOT), str(EXP / "tests/public_router_parity.cpp"),
               str(EXP / "league/king_rc4/source/agent.cpp"), str(EXP / "league/public_router/source/agent.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    result = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True)
    report = {"result": result.stdout.strip(), "error": result.stderr, "source_branches": dict(counts),
              "scope": "8 recorded sequences and12 forced branch sequences,719actions each; active-unit/unused-argument normalization", "command": command}
    (EXP / "results/king_parity.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2), flush=True)
    result.check_returncode()


if __name__ == "__main__":
    main()
