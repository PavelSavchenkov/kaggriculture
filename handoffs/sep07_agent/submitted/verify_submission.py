import argparse
import gzip
import hashlib
import importlib.util
import json
import shutil
import subprocess
import tarfile
import time
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

from kaggle_environments import make
from pack_cases import pack

OUT = Path(__file__).resolve().parent


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def game(job):
    seed, seat, opponent = job
    tag = f'{opponent}_{seed}_{seat}'
    module = load(OUT / 'unpacked/main.py', tag)
    path = OUT / 'checks' / f'{tag}.cases.txt'
    durations = []
    days = []
    choices = []
    with path.open('w') as fixture:
        def ours(obs, cfg):
            started = time.perf_counter()
            action = module.agent(obs, cfg)
            durations.append(time.perf_counter() - started)
            assert len(action['hands']) == len(obs['farms'][seat]['hands'])
            assert len(action['market']) <= cfg['maxMarketOrdersPerTurn']
            fixture.write(pack(obs, action, obs['day'] == 0 and obs['hour'] == 0, full_tiles=True))
            if obs['hour'] == 0 and module._STATE[seat]['day']:
                days.append(obs['day'])
            if obs['day'] == 7 and obs['hour'] == 0:
                choices.extend(module._STATE[seat]['chosen'])
            return action
        if opponent == 'teammate':
            rival = load(OUT / 'teammate_reference/main.py', f'teammate_{tag}').agent
        elif opponent == 'self':
            rival = load(OUT / 'unpacked/main.py', f'self_{tag}').agent
        else:
            rival = 'pass'
        agents = [rival, rival]
        agents[seat] = ours
        env = make('kaggriculture', configuration={'seed': seed}, debug=True)
        env.run(agents)
        assert len(durations) == 719, (tag, len(durations))
        statuses = [s.status for s in env.state]
        assert statuses == ['DONE', 'DONE'], (tag, statuses)
        rewards = [s.reward for s in env.state]
        if opponent != 'teammate':
            (OUT / 'checks' / f'{tag}.replay.json').write_text(json.dumps(env.toJSON()))
    return {'seed': seed, 'seat': seat, 'opponent': opponent, 'rewards': rewards,
            'statuses': statuses, 'win': rewards[seat] > rewards[seat ^ 1],
            'margin': rewards[seat] - rewards[seat ^ 1], 'calls': len(durations),
            'total_agent_seconds': sum(durations), 'max_agent_seconds': max(durations),
            'guarded_days': days, 'chosen': choices, 'fixture': path.name}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--games', type=int, default=32)
    args = parser.parse_args()
    (OUT / 'checks').mkdir(exist_ok=True)
    (OUT / 'unpacked').mkdir(exist_ok=True)
    with tarfile.open(OUT / 'submission.tar.gz', 'r:gz') as archive:
        assert archive.getnames() == ['main.py']
        archive.extractall(OUT / 'unpacked', filter='data')
    assert (OUT / 'unpacked/main.py').read_bytes() == (OUT / 'main.py').read_bytes()
    jobs = [(1153000 + i, seat, 'teammate') for i in range(args.games) for seat in range(2)]
    jobs += [(1154000, seat, opponent) for opponent in ('pass', 'self') for seat in range(2)]
    records = []
    with ProcessPoolExecutor(max_workers=4) as pool:
        for result in pool.map(game, jobs):
            records.append(result)
            print(json.dumps(result), flush=True)
    combined = OUT / 'checks/packed_cases.txt'
    with combined.open('wb') as target:
        for result in records:
            with (OUT / 'checks' / result['fixture']).open('rb') as source:
                shutil.copyfileobj(source, target)
    parity = subprocess.run(['conda', 'run', '-n', 'kaggriculture', str(OUT / 'verify_cpp'), str(combined)], capture_output=True, text=True)
    print(parity.stdout, parity.stderr, flush=True)
    parity.check_returncode()
    teammate = [r for r in records if r['opponent'] == 'teammate']
    report = {'games': records, 'teammate_games': len(teammate), 'wins': sum(r['win'] for r in teammate),
              'mean_margin': sum(r['margin'] for r in teammate) / len(teammate),
              'parity': parity.stdout.strip(), 'fixture_sha256': hashlib.sha256(combined.read_bytes()).hexdigest(),
              'max_agent_seconds': max(r['max_agent_seconds'] for r in records),
              'max_full_game_agent_seconds': max(r['total_agent_seconds'] for r in records)}
    assert report['wins'] > len(teammate) / 2, report
    (OUT / 'packed_validation.json').write_text(json.dumps(report, indent=2) + '\n')
    with combined.open('rb') as source, gzip.open(str(combined) + '.gz', 'wb') as target:
        shutil.copyfileobj(source, target)
    for result in records:
        (OUT / 'checks' / result['fixture']).unlink()
    combined.unlink()


if __name__ == '__main__':
    main()
