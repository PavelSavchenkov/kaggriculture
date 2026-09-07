"""Check original Python actions only on fixed exogenous observations."""
import importlib.util
import json
import subprocess
from collections import Counter
from copy import deepcopy
from pathlib import Path

from verify_public_router import pack

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
SOURCE = EXP / 'research/refresh_1112/notebooks/thomastschinkel/kaggriculture-93-8-win-rate-public-state-router/extracted_main.py'


def main():
    spec = importlib.util.spec_from_file_location('router_v5_reference', SOURCE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    paths = sorted((EXP / 'replays').glob('*replay.json'))[:4]
    replays = [json.loads(p.read_text()) for p in paths]
    fixture = EXP / 'tests/router_v5_cases.txt'
    counts = Counter()
    with fixture.open('w') as out:
        def write(obs, reset):
            action = module.agent(obs)
            if 0 <= obs['step'] < 719:
                counts[f"block{obs['step']//144}_route{module._SESSIONS[obs['player']][1]}"] += 1
            out.write(pack(obs, action, reset, full_tiles=True))
        for r in replays:
            for seat in range(2):
                for step, pair in enumerate(r['steps'][:719]):
                    write(dict(pair[seat]['observation'], step=step), step == 0)
        for branch in range(6):
            for step, pair in enumerate(replays[0]['steps'][:719]):
                obs = deepcopy(pair[0]['observation'])
                obs['step'] = step
                if step >= 144:
                    obs['town']['unlocked_shops'] = [['BAKERY', 'BRUNCH_SPOT'], ['BAKERY', 'PIZZA_SHOP'], ['YARN_STORE', 'SMOOTHIE_SHOP']][branch // 2]
                if step >= 576:
                    obs['market']['prices']['CARROT'] = 54 + branch % 2
                write(obs, step == 0)
        module._SESSIONS.clear()
        for i, step in enumerate([500, 144, 145, 576, 577, 2, 719]):
            obs = deepcopy(replays[0]['steps'][min(step, 718)][0]['observation'])
            obs['step'] = step
            write(obs, i == 0)
    body = (EXP / 'tests/refresh_1010_parity.cpp').read_text()
    body = '#define VERIFY_KING\n#include "../league/public_router_v5/source/agent.hpp"\n#include <fstream>\n#include <iostream>\nint main(int argc,char**argv){if(argc!=2)return 2;std::ifstream in(argv[1]);\ncompositions::public_router_v5::Agent agent;\n' + body[body.index('    int cases = 0, reset;'):]
    cpp = EXP / 'tests/router_v5_parity.cpp'
    cpp.write_text(body)
    binary = EXP / 'build/refresh_1112/router_v5_parity'
    binary.parent.mkdir(exist_ok=True)
    command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(ROOT), str(cpp), str(EXP / 'league/public_router_v5/source/agent.cpp'), '-o', str(binary)]
    subprocess.run(command, check=True)
    result = subprocess.run(['conda', 'run', '-n', 'kaggriculture', str(binary), str(fixture)], check=True, capture_output=True, text=True)
    report = {'result': result.stdout.strip(), 'coverage': counts, 'command': command,
              'scope': 'Eight recorded719-action streams, six forced shop/price streams, seven reset/backward/out-of-range cases; Python only an exogenous action oracle'}
    (EXP / 'results/public_router_v5_parity.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
