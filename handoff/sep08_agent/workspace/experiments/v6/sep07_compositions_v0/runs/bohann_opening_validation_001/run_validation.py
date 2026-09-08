"""Validate the compact component, then execute the preregistered fresh matrix."""
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
SOURCE = EXP / 'runs/fresh_courses_1642'


def main():
    lines = (SOURCE / 'promotion_build.log').read_text().splitlines()
    binary = Path(next(line for line in reversed(lines) if line.startswith(str(EXP / 'build'))))
    assert binary.is_file() and binary.is_relative_to(EXP / 'build')
    spec = json.loads((RUN / 'PREREGISTERED.json').read_text())
    commands = []

    def run(agent, opponent, directory, games, seed, profile=False):
        path = directory / f'{agent}_vs_{opponent}.json'
        assert not path.exists()
        command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', agent, '--b', opponent,
                   '--games', str(games), '--seed-start', str(seed), '--seat-mode', 'both', '--threads', '4',
                   '--validate', '--output', str(path)]
        if profile:
            command.append('--profile')
        commands.append(command)
        with path.with_suffix('.log').open('w') as log:
            subprocess.run(command, check=True, stdout=log, stderr=subprocess.STDOUT)
        return json.loads(path.read_text())

    compact = RUN / 'compact_parity'
    compact.mkdir()
    checked = 0
    for opponent in ['crop_mix_t2_wheat', 'wheat_one_fert', 'teammate_shoprouter', 'public_router', 'king_rc4', 'public_router_v5']:
        result = run(spec['candidate'], opponent, compact, 64, 1000)
        control = json.loads((SOURCE / f'ablation_discovery/fresh_bohann25_opening_vs_{opponent}.json').read_text())
        assert result['games'] == control['games'], opponent
        checked += len(result['games'])
    (RUN / 'COMPACT_PARITY.json').write_text(json.dumps({'complete_records_exact': checked,
        'scope': 'All fields, including both action hashes, cash, production, sales, discards and faults. Six opponents.'}, indent=2) + '\n')
    print('Compact two-sequence component matches', checked, 'full records', flush=True)
    fresh = RUN / 'fresh'; fresh.mkdir()
    tasks = [(agent, opponent) for agent in [spec['candidate'], spec['baseline']] for opponent in spec['opponents']]
    with ThreadPoolExecutor(max_workers=2) as pool:
        futures = [pool.submit(run, agent, opponent, fresh, spec['seeds'], spec['seed_start']) for agent, opponent in tasks]
        for future in futures:
            future.result()
    profiles = RUN / 'profiles'; profiles.mkdir()
    for opponent in ['crop_mix_t2_wheat', 'teammate_shoprouter', 'public_router', 'king_rc4', 'public_router_v5']:
        for agent in [spec['candidate'], spec['baseline']]:
            run(agent, opponent, profiles, 32, 1000, True)
    (RUN / 'COMMANDS.json').write_text(json.dumps({'binary': str(binary), 'commands': commands}, indent=2) + '\n')
    print('Complete fresh matrix:', len(tasks), 'cohorts;', len(tasks) * spec['seeds'] * 2, 'games')


if __name__ == '__main__':
    main()
