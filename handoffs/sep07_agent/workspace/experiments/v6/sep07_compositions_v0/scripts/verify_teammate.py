"""Original Python/native teammate versus C++ on identical observations."""
import hashlib
import importlib.util
import json
import shutil
import subprocess
from copy import deepcopy
from pathlib import Path
from verify_public_router import pack

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    origin = ROOT / "external/kaggriculture/agents/shoprouter-rl-v2"
    target = EXP / "tests/reference/teammate"
    target.mkdir(parents=True, exist_ok=True)
    hashes = {}
    for name in ("main.py", "_shop_bridge.py", "agent.so"):
        shutil.copy2(origin / name, target / name)
        hashes[name] = hashlib.sha256((target / name).read_bytes()).hexdigest()
    (target / "IMPORT.json").write_text(json.dumps({"origin": str(origin.relative_to(ROOT)), "sha256": hashes}, indent=2) + "\n")
    spec = importlib.util.spec_from_file_location("teammate_reference", target / "main.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    fixture = EXP / "tests/teammate_cases.txt"
    paths = sorted((EXP / "replays").glob("*replay.json"))[:4]
    with fixture.open("w") as out:
        for path in paths:
            replay = json.loads(path.read_text())
            for seat in range(2):
                for step, pair in enumerate(replay["steps"][:719]):
                    # Replay JSON omits shared step in seat 1. The live runner
                    # hydrates shared observation fields before calling agents.
                    obs = dict(pair[seat]["observation"], step=step)
                    out.write(pack(obs, module.agent(obs), step == 0))
        replay = json.loads(paths[0].read_text())
        for route in range(2):
            for step, pair in enumerate(replay["steps"][:719]):
                obs = deepcopy(pair[0]["observation"])
                obs["step"] = step
                if step == 360:
                    obs["town"]["unlocked_shops"] = ["BAKERY"]
                    obs["market"]["inventory"]["FERTILIZER"] = 10232 if route else 10233
                out.write(pack(obs, module.agent(obs), step == 0))
    binary = EXP / "build/teammate_parity"
    command = ["conda", "run", "-n", "kaggriculture", "g++", "-std=c++20", "-O2", "-DVERIFY_TEAMMATE", "-I", str(ROOT),
               str(EXP / "tests/public_router_parity.cpp"), str(EXP / "league/teammate_shoprouter/source/agent.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    result = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True)
    if result.returncode:
        print(result.stdout, result.stderr)
        result.check_returncode()
    report = {"result": result.stdout.strip(), "scope": "8 recorded sequences plus 2 forced-route sequences; 719 turns each",
              "normalization": "Hydrate shared step omitted by seat-1 replay JSON; active workers only; ignore unused opcode arguments and quantities", "source_sha256": hashes,
              "command": command}
    (EXP / "results/teammate_parity.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
