"""Rebuild the actual dependency snapshot and compare full active-branch games."""
from pathlib import Path
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]


def main():
    binary = ROOT / json.loads((RUN / 'FRESH_BUILD.json').read_text())['binary']
    frozen = RUN / 'frozen'
    command = ['conda', 'run', '-n', 'kaggriculture', 'python', str(EXP / 'scripts/freeze_arena_inputs.py'), str(binary), str(frozen)]
    subprocess.run(command, check=True)
    report = json.loads((frozen / 'FROZEN.json').read_text())
    with (frozen / 'rebuild.log').open('w') as log:
        subprocess.run(report['rebuild_command'], stdout=log, stderr=subprocess.STDOUT, check=True)
    output = frozen / 'rebuilt_vs_public_router_v52.json'
    command = ['conda', 'run', '-n', 'kaggriculture', str(frozen / 'arena_rebuilt'),
               '--a', 'rival_wool_context_v3', '--b', 'public_router_v52', '--games', '128',
               '--seed-start', '1954000', '--seat-mode', 'both', '--threads', '4',
               '--profile', '--validate', '--native-shops', '--output', str(output)]
    subprocess.run(command, check=True)
    actual = json.loads(output.read_text())['games']
    expected = json.loads((RUN / 'native/rival_wool_context_v3_vs_public_router_v52.json').read_text())['games']
    control = json.loads((RUN / 'native/wool_family_context_v2_vs_public_router_v52.json').read_text())['games']
    assert actual == expected
    changed = sum(a['action_hash'] != b['action_hash'] for a, b in zip(actual, control, strict=True))
    assert changed > 0
    result = {'frozen_full_records_equal': True, 'frozen_games': len(actual),
              'active_branch_changed_games': changed, 'opponent': 'public_router_v52',
              'dependencies': len(report['files_sha256']), 'command': command}
    (RUN / 'FINAL_AUDITS.json').write_text(json.dumps(result, indent=2) + '\n')
    print(result)


if __name__ == '__main__':
    main()
