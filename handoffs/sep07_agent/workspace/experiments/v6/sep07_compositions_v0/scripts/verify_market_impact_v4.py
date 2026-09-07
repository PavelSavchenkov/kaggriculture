"""Frozen Python oracle only on externally supplied observations."""
import importlib.util
import json
import subprocess
from copy import deepcopy
from pathlib import Path

from verify_public_router import pack

EXP = Path(__file__).resolve().parents[1]


def main():
    source = EXP / 'research/refresh_1010/notebooks/y3uanm/kaggriculture-market-impact-router-v4/extracted_main.py'
    spec = importlib.util.spec_from_file_location('market_v4_reference', source)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    paths = sorted((EXP / 'replays').glob('*replay.json'))[:8]
    replays = [json.loads(path.read_text()) for path in paths]
    fixture = EXP / 'tests/market_impact_v4_cases.txt'
    with fixture.open('w') as out:
        for replay in replays:
            for seat in range(2):
                agent = module.Agent()
                for step, pair in enumerate(replay['steps'][:719]):
                    obs = dict(pair[seat]['observation'], step=step)
                    out.write(pack(obs, agent.act(obs), step == 0, full_tiles=True))
        for branch in range(4):
            agent = module.Agent()
            for step, pair in enumerate(replays[branch]['steps'][:719]):
                obs = deepcopy(pair[0]['observation'])
                obs['step'] = step
                obs['town']['unlocked_shops'] = [] if step < 72 else ['YARN_STORE' if branch % 2 else 'BAKERY']
                if branch >= 2:
                    obs['private']['shed'] = {'CARROT': 30, 'TOMATO': 30, 'MILK': 20, 'WOOL': 20}
                    obs['market']['prices']['CARROT'] = 600
                    obs['market']['prices']['TOMATO'] = 2000
                out.write(pack(obs, agent.act(obs), step == 0, full_tiles=True))
    command = ['conda', 'run', '-n', 'kaggriculture', str(EXP / 'build/refresh_1010/market_v4_parity'), str(fixture)]
    result = subprocess.run(command, capture_output=True, text=True)
    print(result.stdout, result.stderr)
    result.check_returncode()
    (EXP / 'results/market_impact_v4_parity.json').write_text(json.dumps({'result': result.stdout.strip(), 'scope': 'Sixteen recorded streams plus four forced shop/stock/price streams, including positive-ranking cases; no Python local gameplay', 'command': command}, indent=2) + '\n')


if __name__ == '__main__':
    main()
