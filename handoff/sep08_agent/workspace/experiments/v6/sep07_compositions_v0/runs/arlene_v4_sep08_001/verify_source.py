"""Compare the public Python reference with the typed C++ port."""
from collections import Counter
from copy import deepcopy
from datetime import datetime, timezone
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

reference = EXP / json.loads((RUN / 'LINEAGE.json').read_text())['reference']
spec = importlib.util.spec_from_file_location('arlene_v4_reference', reference)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
counts = Counter()
routes = Counter()


class Observed(module.Agent):
    def _budget_guard(self, obs, farm, invs, shed, prices, market, step):
        result = super()._budget_guard(obs, farm, invs, shed, prices, market, step)
        counts['budget_checks'] += step % 72 == 0
        counts['budget_changed'] += result != market
        return result


replays = [json.loads(p.read_text()) for p in sorted((EXP / 'replays').glob('*replay.json'))[:4]]
fixture = RUN / 'source_cases.txt'
with fixture.open('w') as out:
    for replay in replays:
        for seat in range(2):
            policy = Observed()
            for step, pair in enumerate(replay['steps'][:719]):
                obs = dict(pair[seat]['observation'], step=step)
                action = policy.act(obs)
                routes[policy.cur] += 1
                counts['recorded_cases'] += 1
                out.write(pack(obs, action, step == 0, full_tiles=True))
    for branch in range(4):
        policy = Observed()
        for step, pair in enumerate(replays[0]['steps'][:719]):
            obs = deepcopy(pair[0]['observation'])
            obs['step'] = step
            if step == 226:
                obs['town']['unlocked_shops'] = ['YARN_STORE' if branch in (1, 2) else 'BAKERY']
            if step == 360:
                obs['market']['prices']['CARROT'] = 42 if branch == 2 else 41
            if step == 433:
                obs['market']['inventory']['MILK'] = 10067 if branch == 3 else 10066
            if step % 72 == 0 or step % 24 == 23 or step == 718:
                obs['farms'][obs['player']]['money'] = 0
                obs['private']['shed'] = {item: 11 for item in module.PRODUCTS}
                obs['private']['inventories'] = [{'WOOL': 3, 'FERTILIZER': 2} for _ in obs['private']['inventories']]
            if step == 718:
                obs['market']['prices'] = {item: 1 for item in module.PRODUCTS}
            action = policy.act(obs)
            counts['forced_cases'] += 1
            counts['zero_sale_slots'] += sum(o[0] == 'SELL' and o[2] == 0 for o in action['market'])
            routes[policy.cur] += 1
            out.write(pack(obs, action, step == 0, full_tiles=True))
assert len(routes) == 4 and counts['budget_changed'] > 0 and counts['zero_sale_slots'] > 0
source = (EXP / 'tests/public_router_parity.cpp').read_text()
source = '#define VERIFY_CAPACITY 1\n#include "source/agent.hpp"\n' + source[source.index('#include <fstream>'):]
source = source.replace('compositions::public_capacity_router::AgentCore agent(VERIFY_CAPACITY);', 'compositions::arlene_v4_sep08::AgentCore agent(31);')
(RUN / 'parity.cpp').write_text(source)
commands = [
    ['conda', 'run', '-n', 'kaggriculture', 'g++', '-std=c++20', '-O2', '-I', str(ROOT),
     str(RUN / 'parity.cpp'), str(RUN / 'source/agent.cpp'), '-o', str(RUN / 'parity')],
    ['conda', 'run', '-n', 'kaggriculture', str(RUN / 'parity'), str(fixture)],
]
for i, command in enumerate(commands):
    result = subprocess.run(command, capture_output=True, text=True)
    (RUN / f'parity_command{i}.log').write_text(result.stdout + result.stderr)
    result.check_returncode()
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'counts': dict(counts),
    'route_actions': dict(routes), 'result': result.stdout.strip(), 'commands': commands,
    'scope': 'Eight full recorded observation sequences and four full forced route/capacity/budget/floor-terminal sequences. Direct reference method, no exception fallback. Source parity, not competitive strength.'}
(RUN / 'SOURCE_PARITY.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
