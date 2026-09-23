"""Freeze a replay cohort, independently validate native exports, and pack arrays."""
import argparse
import hashlib
import json
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--name', required=True)
    parser.add_argument('--workers', type=int, default=8)
    args = parser.parse_args()
    target = ROOT / 'data' / args.name
    target.mkdir(exist_ok=False)
    for name in ('perspectives.json', 'all.txt', 'train.txt', 'validation.txt', 'development.txt'):
        shutil.copy2(ROOT / 'data' / name, target / name)
    rows = json.loads((target / 'perspectives.json').read_text())
    if any(r['split'] == 'confirmation' for r in rows):
        raise ValueError('Protected confirmation episodes must remain unopened')
    lines = (target / 'all.txt').read_text().splitlines()
    binary = target / 'bc_export'
    shutil.copy2(ROOT / 'build_native/bc_export', binary)
    shutil.copy2(ROOT / 'snapshots/compiler_current.json', target / 'compiler.json')
    sources = [binary, ROOT / 'source/export.cpp', ROOT / 'source/features.hpp', ROOT / 'source/labels.hpp',
               target / 'compiler.json', target / 'perspectives.json']
    (target / 'provenance.json').write_text(json.dumps({str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                                                       for p in sources}, indent=2)+'\n')
    for index in range(args.workers):
        (target / f'shard{index}.txt').write_text('\n'.join(lines[index::args.workers])+'\n')
    def export(index):
        with (ROOT / 'reports' / f'{args.name}_export_{index}.log').open('w') as log:
            subprocess.run([str(binary), str(target / f'shard{index}.txt'), str(target / f'shard{index}.bin'),
                            'observed-compatible'], cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, check=True)
        print(f'completed shard {index}', flush=True)
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        list(pool.map(export, range(args.workers)))
    subprocess.run(['python', '-m', 'bc.data', *[str(target / f'shard{i}.bin') for i in range(args.workers)],
                    '--manifest', str(target / 'perspectives.json'), '--output', str(target / 'arrays')], cwd=ROOT, check=True)


if __name__ == '__main__':
    main()
