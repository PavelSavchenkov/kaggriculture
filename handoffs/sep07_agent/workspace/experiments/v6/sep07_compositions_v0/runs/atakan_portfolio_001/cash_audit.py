"""Compile a C++ diagnostic against frozen policies and measure exact market cash."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

RUN = Path(__file__).resolve().parent
RIVALS = ['animal_adaptive_r1_c0_b0', 'teammate_shoprouter', 'public_router_v5', 'king_rc4', 'public_router']


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--all', action='store_true')
    args = parser.parse_args()
    build = RUN / 'build/generic'
    manifest = json.loads((build / 'BUILD.json').read_text())
    command = manifest['binaries'][1]['command'].copy()
    source = RUN / 'source/cash_audit.cpp'
    binary = build / 'cash_audit'
    command = [str(source) if x.endswith('/source/diagnostics.cpp') else x for x in command]
    command[-1] = str(binary)
    subprocess.run(command, check=True)
    records = [{'command': command, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                'data_sha256': hashlib.sha256((source.parent / 'data.inc').read_bytes()).hexdigest(),
                'frozen_dependency_manifest': str(build / 'BUILD.json')}]
    for rival in RIVALS if args.all else ['public_router_v5']:
        for agent in ['atakan_cow', 'atakan_sheep', 'atakan_goose'] if args.all else ['atakan_cow', 'atakan_goose']:
            output = RUN / 'results' / f'cash_{agent}_vs_{rival}.json'
            command = ['conda', 'run', '-n', 'kaggriculture', str(binary), '--a', agent, '--b', rival,
                       '--seed-start', '1000' if args.all else '1028', '--games', '32' if args.all else '1',
                       '--seat-mode', 'both' if args.all else '0', '--output', str(output)]
            subprocess.run(command, check=True)
            records.append({'command': command, 'output_sha256': hashlib.sha256(output.read_bytes()).hexdigest()})
    (RUN / 'CASH_AUDIT_COMMANDS.json').write_text(json.dumps(records, indent=2) + '\n')


if __name__ == '__main__':
    main()
