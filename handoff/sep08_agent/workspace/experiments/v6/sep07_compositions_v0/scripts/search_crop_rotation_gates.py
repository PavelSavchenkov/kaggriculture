"""Export exact leaf outcomes, then run the bounded C++ observed-shop search."""
import argparse
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('run')
    args = parser.parse_args()
    out = EXP / 'runs' / args.run
    assert not out.exists()
    out.mkdir()
    source = EXP / 'runs/crop_rotation_league_001'
    names = ['investment_context_guarded_001_best', 'teammate_shoprouter', 'public_router', 'king_rc4', 'public_router_v5']
    files = []
    with (out / 'cases.txt').open('w') as target:
        target.write(str(len(names)) + '\n' + '\n'.join(names) + '\n')
        for index, name in enumerate(names):
            pair = [source / f'{agent}_vs_{name}.json' for agent in ['investment_context_guarded_001_best', 'crop_rotation_009']]
            files.extend(pair)
            before, after = [json.loads(p.read_text())['games'] for p in pair]
            assert len(before) == len(after) == 512
            for a, b in zip(before, after):
                assert (a['seed'], a['seat'], a['shops'][:4]) == (b['seed'], b['seat'], b['shops'][:4])
                row = [index, a['seed'], a['seat'], *a['shops'][:4], a['cash'], a['opponent_cash'], b['cash'], b['opponent_cash']]
                target.write(' '.join(map(str, row)) + '\n')
    cpp = EXP / 'src/search_crop_rotation_gates.cpp'
    binary = EXP / 'build/search_crop_rotation_gates'
    shutil.copy2(cpp, out / cpp.name)
    command = ['conda', 'run', '-n', 'kaggriculture', str(binary), str(out / 'cases.txt'), str(out / 'gates.csv')]
    files += [cpp, binary, Path(__file__), ROOT / 'fast_game_engine/sim.hpp', out / 'cases.txt']
    record = {'command': command, 'objective': 'Equal weight per five discovery opponents; tie-half win utility, margin tie-break.',
              'scope': '25 gates over first4revealed shops only. Exact fixed-course leaves share the unchanged prefix through day12 entry; executable gate parity and fresh testing are still required.',
              'files_sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
    (out / 'RUN.json').write_text(json.dumps(record, indent=2) + '\n')
    subprocess.run(command, check=True)


if __name__ == '__main__':
    main()
