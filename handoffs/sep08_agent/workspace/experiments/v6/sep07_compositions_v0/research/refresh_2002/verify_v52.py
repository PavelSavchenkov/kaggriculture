"""Compare typed C++ output with the audited source callable on fixed observations."""
from pathlib import Path
from collections import Counter
from copy import deepcopy
import importlib.util
import json
import subprocess
import sys

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from verify_public_router import pack

source = RUN / 'notebooks/thomastschinkel/kaggriculture-95-5-win-rate-via-replay-routing/extracted_main.py'
spec = importlib.util.spec_from_file_location('v52_reference', source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
paths = sorted((EXP / 'replays').glob('*replay.json'))[:4]
replays = [json.loads(path.read_text()) for path in paths]
folder = RUN / 'v52_parity'
folder.mkdir(exist_ok=False)
fixture = folder / 'cases.txt'
counts = Counter()
with fixture.open('w') as output:
    def write(obs, reset):
        action = module.agent(obs)
        if 0 <= obs['step'] < 719:
            counts[f"block{obs['step']//72}_route{module.sessions[obs['player']]['route']}"] += 1
        output.write(pack(obs, action, reset, full_tiles=True))
    for replay in replays:
        for seat in range(2):
            for step, pair in enumerate(replay['steps'][:719]):
                write(dict(pair[seat]['observation'], step=step), step == 0)
    for shops in [['BAKERY', 'BRUNCH_SPOT'], ['BAKERY', 'PIZZA_SHOP'], ['YARN_STORE', 'SMOOTHIE_SHOP']]:
        for tomato_inventory in [9915, 9916, 9917]:
            for step, pair in enumerate(replays[0]['steps'][:719]):
                obs = deepcopy(pair[0]['observation']);obs['step'] = step
                if step >= 144:
                    obs['town']['unlocked_shops'] = shops
                if step >= 288:
                    obs['market']['inventory']['TOMATO'] = tomato_inventory
                write(obs, step == 0)
    module.sessions.clear()
    for i, step in enumerate([500, 144, 145, 288, 289, 2, 719]):
        obs = deepcopy(replays[0]['steps'][min(step, 718)][0]['observation']);obs['step'] = step
        write(obs, i == 0)
body = (EXP / 'tests/refresh_1010_parity.cpp').read_text()
code = '#define VERIFY_KING\n#include "experiments/v6/sep07_compositions_v0/league/public_router_v52/source/agent.hpp"\n#include <fstream>\n#include <iostream>\nint main(int argc,char**argv){if(argc!=2)return 2;std::ifstream in(argv[1]);\nkag::agents::public_router_v52::Agent agent;\n'
code += body[body.index('    int cases = 0, reset;'):]
cpp = folder / 'parity.cpp';cpp.write_text(code)
binary = folder / 'parity'
command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(ROOT),
           str(cpp), str(EXP / 'league/public_router_v52/source/agent.cpp'), '-o', str(binary)]
subprocess.run(command, check=True)
result = subprocess.run(['conda', 'run', '-n', 'kaggriculture', str(binary), str(fixture)], capture_output=True, text=True, check=True)
report = {'result': result.stdout.strip(), 'coverage': counts, 'command': command,
          'scope': 'Eight recorded719-action streams, nine forced shop/tomato-inventory streams including exact threshold9916, seven reset/backward/out-of-range cases. Source Python only used as a fixed-observation oracle.'}
(RUN / 'V52_PARITY.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
