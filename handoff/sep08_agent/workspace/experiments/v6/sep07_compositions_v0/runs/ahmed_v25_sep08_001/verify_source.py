"""Source-oracle parity on recorded and forced branch/layer observations."""
from collections import Counter
from copy import deepcopy
from pathlib import Path
import importlib.util
import json
import subprocess
import sys

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from verify_public_router import pack

source = EXP / 'research/refresh_sep08_1508/notebooks/ahmed/extracted/main.py'
spec = importlib.util.spec_from_file_location('v25_reference', source)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
chassis = module._IMPL.chassis
counts, layers = Counter(), Counter()
for name in ['_weed_repair', '_sell_lead', '_budget_guard', '_room_guard', '_clamp_sells', '_dead_stock', '_terminal_liquidation']:
    original = getattr(chassis, name)
    def measured(action, *args, original=original, name=name):
        before = deepcopy(action)
        result = original(action, *args)
        layers[name] += action != before
        return result
    setattr(chassis, name, measured)

folder = RUN / 'source_parity'
folder.mkdir(exist_ok=True)
fixture = folder / 'cases.txt'
paths = sorted((EXP / 'replays').glob('*replay.json'))[:4]
replays = [json.loads(p.read_text()) for p in paths]
with fixture.open('w') as output:
    def write(obs, reset):
        if reset: chassis.players.clear()
        action = module.agent(obs)
        assert not any(chassis.diagnostics.values()), chassis.diagnostics
        route = chassis.players[obs['player']]['route']
        counts[f'route{route}'] += 1
        counts['maximum_pending'] = max(counts['maximum_pending'], max((len(v) for v in chassis.players[obs['player']]['pending'].values()), default=0))
        for i, order in enumerate(action['market']):
            if not order or order[0] == 'PASS' or (order[0] not in ['HIRE', 'BUY_LAND'] and int(order[2]) <= 0):
                action['market'][i] = ['SELL', 'WHEAT', 0]
        output.write(pack(obs, action, reset, full_tiles=True))
    for replay in replays:
        for seat in range(2):
            for step, pair in enumerate(replay['steps'][:719]):
                write(dict(pair[seat]['observation'], step=step), step == 0)
    for shops in [['BAKERY', 'BRUNCH_SPOT'], ['YARN_STORE', 'SMOOTHIE_SHOP']]:
        for inventory in [9887, 9888, 9889]:
            for step, pair in enumerate(replays[0]['steps'][:719]):
                obs = deepcopy(pair[0]['observation']);obs['step'] = step
                if step >= 144: obs['town']['unlocked_shops'] = shops
                if step >= 648: obs['market']['inventory']['EGG'] = inventory
                write(obs, step == 0)
    for step, pair in enumerate(replays[0]['steps'][:719]):
        obs = deepcopy(pair[0]['observation']);obs['step'] = step
        obs['farms'][0]['money'] = 0
        obs['private']['shed'] = {'WHEAT': 20, 'FERTILIZER': 20, 'WOOL': 20, 'MILK': 20, 'EGG': 20}
        write(obs, step == 0)
    for i, step in enumerate([500, 144, 145, 288, 289, 289, 2, 719]):
        obs = deepcopy(replays[0]['steps'][min(step, 718)][0]['observation']);obs['step'] = step
        write(obs, i == 0)

body = (EXP / 'tests/refresh_1010_parity.cpp').read_text()
code = '#define VERIFY_KING\n#include "experiments/v6/sep07_compositions_v0/runs/ahmed_v25_sep08_001/proposals/ahmed_v25/source/agent.hpp"\n#include <fstream>\n#include <iostream>\nint main(int argc,char**argv){if(argc!=2)return 2;std::ifstream in(argv[1]);\ncompositions::ahmed_v25::Agent agent;\n'
code += body[body.index('    int cases = 0, reset;'):]
cpp = folder / 'parity.cpp';cpp.write_text(code)
binary = folder / 'parity'
command = ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(ROOT), str(cpp), str(EXP / 'runs/ahmed_v25_sep08_001/proposals/ahmed_v25/source/agent.cpp'), '-o', str(binary)]
result = subprocess.run(command, capture_output=True, text=True)
(folder / 'build.log').write_text(result.stdout + result.stderr)
result.check_returncode()
result = subprocess.run(['conda', 'run', '-n', 'kaggriculture', str(binary), str(fixture)], capture_output=True, text=True)
report = {'returncode': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr, 'coverage': counts,
          'layer_changes': layers, 'python_fallbacks': chassis.diagnostics, 'command': command,
          'scope': 'Eight recorded streams, six forced shop/egg thresholds, low-cash/full-shed stream, eight late/reset/repeated/out-of-range observations. Python source is only a fixed-observation oracle.'}
(RUN / 'SOURCE_PARITY.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
result.check_returncode()
