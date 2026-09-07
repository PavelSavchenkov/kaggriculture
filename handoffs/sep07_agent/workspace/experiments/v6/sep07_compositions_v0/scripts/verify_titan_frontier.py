"""Use frozen Python only on exogenous observations to check the C++ port."""
import argparse
import importlib.util
import json
import subprocess
import sys
from collections import Counter
from copy import deepcopy
from pathlib import Path

from verify_public_router import pack

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]
SOURCE = EXP / 'research/refresh_1010/notebooks/tokenjunkielabs/titan-kaggriculture-frontier-source'
CFG = {'turnsPerDay': 24, 'townShopSellInterval': 4, 'townCenterSellInterval': 24,
       'maxMarketOrdersPerTurn': 10, 'shedCapacity': 100}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--parent', action='store_true')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('titan_reference', SOURCE / 'extracted_main.py')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    paths = sorted((EXP / 'replays').glob('*replay.json'))[:4]
    replays = [json.loads(path.read_text()) for path in paths]
    name = 'kaito_v43' if args.parent else 'titan_frontier'
    fixture = EXP / f'tests/{name}_cases.txt'
    counts = Counter()
    with fixture.open('w') as out:
        def write(obs, step):
            previous = 0 if step == 0 else module._FRONTIER_STATE.get(obs['player'], {}).get('changed_market_turns', 0)
            action = module._FRONTIER_PARENT(obs, CFG) if args.parent else module.agent(obs, CFG)
            if not args.parent:
                state = module._FRONTIER_STATE[obs['player']]
                counts[state['sale_regime']] += 1
                counts['changed_market_turns'] += state['changed_market_turns'] > previous
            counts['route_' + module._V43_POLICY.states[obs['player']]['route']] += 1
            out.write(pack(obs, action, step == 0, full_tiles=True))
        for replay in replays:
            for seat in range(2):
                for step, pair in enumerate(replay['steps'][:719]):
                    write(dict(pair[seat]['observation'], step=step), step)
        for branch in range(8):
            for step, pair in enumerate(replays[branch % 4]['steps'][:719]):
                obs = deepcopy(pair[0]['observation'])
                obs['step'] = step
                obs['town']['unlocked_shops'] = ([] if step < 72 else ['YARN_STORE' if branch % 2 else 'BAKERY']) + ([] if step < 144 else ['BAKERY' if branch % 2 else 'YARN_STORE'])
                obs['farms'][1] = deepcopy(obs['farms'][0])
                if branch in (2, 3):
                    obs['farms'][1]['hands'].extend([[4, 4]] * (branch - 1))
                if branch >= 4:
                    obs['private']['shed'] = {'MILK': 12, 'WOOL': 12, 'STRAWBERRY': 12, 'MELON': 12, 'EGG': 12, 'WHEAT': 15}
                if branch >= 6 and 144 <= step < 168:
                    for farm in obs['farms']:
                        for x, y in [farm['farmer'], *farm['hands']]:
                            farm['tiles'][y][x] = {'kind': 'WEED'}
                write(obs, step)
    binary = EXP / 'build/refresh_1010' / ('kaito_v43_parity' if args.parent else 'titan_parity')
    result = subprocess.run(['conda', 'run', '-n', 'kaggriculture', str(binary), str(fixture)], capture_output=True, text=True)
    print(result.stdout, result.stderr)
    result.check_returncode()
    report = {'result': result.stdout.strip(), 'coverage': counts, 'config': CFG,
              'scope': 'Eight recorded streams plus eight forced shop/similarity/stock/weed streams; no Python local gameplay'}
    (EXP / f'results/{name}_parity.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
