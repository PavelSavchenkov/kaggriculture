"""Run the full common-seed component matrix using one compiled C++ arena."""
import json
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]


def main():
    lines = (RUN / 'ablation_build.log').read_text().splitlines()
    binary = Path(next(line for line in reversed(lines) if line.startswith(str(EXP / 'build'))))
    assert binary.is_file() and binary.is_relative_to(EXP / 'build')
    out = RUN / 'ablation_discovery'
    assert not out.exists()
    out.mkdir()
    agents = ['crop_mix_t2_wheat', 'fresh_bohann25_opening', 'fresh_bohann25_suffix1',
              'fresh_bohann25_suffix6', 'fresh_bohann25_opening_suffix6']
    opponents = ['crop_mix_t2_wheat', 'wheat_one_fert', 'teammate_shoprouter', 'public_router', 'king_rc4', 'public_router_v5']
    commands = []
    for agent in agents:
        for opponent in opponents:
            stem = f'{agent}_vs_{opponent}'
            command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', agent, '--b', opponent,
                       '--games', '64', '--seed-start', '1000', '--seat-mode', 'both', '--threads', '4',
                       '--validate', '--output', str(out / (stem + '.json'))]
            commands.append({'command': command, 'log': str(out / (stem + '.log'))})
    (out / 'COMMANDS.json').write_text(json.dumps(commands, indent=2) + '\n')

    def run(record):
        with Path(record['log']).open('w') as log:
            subprocess.run(record['command'], check=True, stdout=log, stderr=subprocess.STDOUT)

    with ThreadPoolExecutor(max_workers=2) as pool:
        for _ in pool.map(run, commands):
            pass
    print('Completed', len(commands), 'paired cohorts,', len(commands) * 128, 'games')


if __name__ == '__main__':
    main()
