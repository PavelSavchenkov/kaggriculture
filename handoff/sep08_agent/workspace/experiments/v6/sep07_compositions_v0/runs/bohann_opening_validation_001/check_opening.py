"""Operational and native-RNG checks for a complete conditional crop policy."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('run')
    parser.add_argument('--agent', required=True)
    parser.add_argument('--parent', default='crop_value_m2_t4')
    parser.add_argument('--generic', type=Path, required=True)
    parser.add_argument('--pair', type=Path, required=True)
    parser.add_argument('--debug', type=Path, required=True)
    parser.add_argument('--native-seed', type=int, default=1690000)
    args = parser.parse_args()
    output = EXP / 'runs' / args.run
    assert not output.exists()
    output.mkdir()
    records = {'binaries': {key: {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
               for key, path in [('generic', args.generic), ('pair', args.pair), ('debug', args.debug)]}}

    def run(name, own, opponent, binary=args.generic, games=32, seed=1000, threads=6, extra=()):
        path = output / (name + '.json')
        command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', own, '--b', opponent,
                   '--games', str(games), '--seed-start', str(seed), '--seat-mode', 'both',
                   '--threads', str(threads), '--budget-expansions', '100000', '--validate',
                   '--output', str(path), *extra]
        with (output / (name + '.log')).open('w') as log:
            subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, check=True)
        data = json.loads(path.read_text())
        assert len(data['games']) == 2 * games and all(g['turns'] == 719 for g in data['games'])
        records[name] = {'command': command, 'summary': {k: v for k, v in data.items() if k != 'games'},
                         'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
        (output / 'RUN.json').write_text(json.dumps(records, indent=2) + '\n')
        return data['games']

    generic = run('generic64', args.agent, args.parent)
    pair = run('pair64', args.agent, args.parent, binary=args.pair)
    debug = run('debug64', args.agent, args.parent, binary=args.debug)
    serial = run('thread64', args.agent, args.parent, threads=1)
    assert generic == pair == debug == serial
    run('self32', args.agent, args.agent, games=16)
    for agent in [args.agent, args.parent]:
        run(agent + '_pass256', agent, 'pass', games=128, seed=args.native_seed)
        for opponent in [args.parent, 'teammate_shoprouter', 'king_rc4', 'public_router', 'public_router_v5']:
            run(agent + '_native_' + opponent, agent, opponent, games=128, seed=args.native_seed,
                extra=['--native-shops'])
    records['generic_pair_debug_thread_all_game_records_equal'] = True
    records['static_review'] = 'The compact wrapper owns one current base instance, resets it and changes only the two public market sequences at steps0and1. All adaptive crop/animal controllers and immutable schedules are inherited. No mutable global state, environment seed, Sim or rival-private input is exposed. Equal observation histories produce equal actions. Source guards belong only to the discarded whole-course ablations.'
    (output / 'CHECKS.json').write_text(json.dumps(records, indent=2) + '\n')
    print('All complete operational checks passed:', args.agent)


if __name__ == '__main__':
    main()
