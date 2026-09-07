"""Differential public-source verification on exogenous observations only."""
import importlib.util
import json
import subprocess
from copy import deepcopy
from pathlib import Path
from verify_public_router import pack

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    reference = EXP / "tests/reference/finance7/main.py"
    spec = importlib.util.spec_from_file_location("finance7_reference", reference)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    replays = [json.loads(file.read_text()) for file in sorted((EXP / "replays").glob("*replay.json"))[:4]]
    reports = []
    for mode in range(4):
        module.L_MFR = bool(mode & 1)
        module.L_FINANCE = bool(mode & 2)
        fixture = EXP / f"tests/finance7_{mode}_cases.txt"
        fired = {"mirror": 0, "finance": 0}
        finance = module._finance_action
        mirror = module._mfr_action

        def count_finance(action, obs, seat):
            old = deepcopy(action)
            result = finance(action, obs, seat)
            fired["finance"] += result != old
            return result

        def count_mirror(action, step):
            old = deepcopy(action)
            result = mirror(action, step)
            fired["mirror"] += result != old
            return result

        module._finance_action = count_finance
        module._mfr_action = count_mirror
        with fixture.open("w") as out:
            for replay in replays:
                for seat in range(2):
                    for step, pair in enumerate(replay["steps"][:719]):
                        obs = dict(pair[seat]["observation"], step=step)
                        out.write(pack(obs, module.agent_v738(obs), step == 0, full_tiles=True))
            for branch in range(4):
                for step, pair in enumerate(replays[0]["steps"][:719]):
                    obs = deepcopy(pair[0]["observation"])
                    obs["step"] = step
                    obs["farms"][1] = deepcopy(obs["farms"][0])
                    if branch == 2 and step % 10 == 0:
                        obs["farms"][1]["money"] += 1
                    if step == 360:
                        obs["town"]["unlocked_shops"] = ["BAKERY"]
                        obs["market"]["inventory"]["FERTILIZER"] = 10232 if branch % 2 else 10233
                    if step in [24, 48, 72, 144, 288] and branch == 3:
                        obs["farms"][0]["money"] = 0
                        obs["private"]["shed"] = {"WHEAT": 3, "CARROT": 3}
                        obs["market"]["prices"]["WHEAT"] = 28
                        obs["market"]["prices"]["CARROT"] = 28
                    out.write(pack(obs, module.agent_v738(obs), step == 0, full_tiles=True))
        module._finance_action, module._mfr_action = finance, mirror
        if mode & 1:
            assert fired["mirror"] > 0
        if mode & 2:
            assert fired["finance"] > 0
        binary = EXP / f"build/finance7_{mode}_parity"
        command = ["conda", "run", "-n", "kaggriculture", "g++", "-std=c++20", "-O2", f"-DVERIFY_FINANCE7={mode}",
                   "-I", str(ROOT), str(EXP / "tests/public_router_parity.cpp"),
                   str(EXP / "league/destbreso_finance7/source/agent.cpp"), "-o", str(binary)]
        subprocess.run(command, check=True)
        result = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True)
        print(mode, result.stdout, result.stderr, fired, flush=True)
        result.check_returncode()
        reports.append({"mode": mode, "result": result.stdout.strip(), "source_layer_firings": fired,
            "scope": "8 recorded sequences plus4 forced mirror/route/funding sequences,719turns each; active-unit/unused-argument normalization", "command": command})
        (EXP / "results/finance7_parity.json").write_text(json.dumps(reports, indent=2) + "\n")


if __name__ == "__main__":
    main()
