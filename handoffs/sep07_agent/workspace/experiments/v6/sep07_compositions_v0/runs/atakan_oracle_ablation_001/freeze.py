"""Freeze the oracle diagnostic report and prove its source experiment unchanged."""
import hashlib
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent
SOURCE = RUN.parent / 'atakan_portfolio_001'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    previous = json.loads((SOURCE / 'FINAL_VALIDATION.json').read_text())
    unchanged = 0
    for relative, expected in previous['artifact_sha256'].items():
        assert digest(SOURCE / relative) == expected, relative
        unchanged += 1
    report = json.loads((RUN / 'REPORT.json').read_text())
    assert report['baseline_source_exact_comparisons'] == 960
    assert report['prefix_state_parity'] == 320
    artifacts = sorted(p for p in RUN.rglob('*') if p.is_file() and '__pycache__' not in p.parts and p.name != 'FINAL_VALIDATION.json')
    command_prefix = ['conda', 'run', '-n', 'kaggriculture', 'python']
    result = {'scope': 'Offline oracle diagnostic only. No deployable policy or promotion.',
              'unchanged_source_experiment_artifacts': unchanged,
              'source_experiment_final_manifest_sha256': digest(SOURCE / 'FINAL_VALIDATION.json'),
              'commands': [command_prefix + [str(RUN / script)] for script in ['prepare.py', 'run.py', 'analyze.py', 'freeze.py']],
              'artifact_sha256': {str(p.relative_to(RUN)): digest(p) for p in artifacts}}
    (RUN / 'FINAL_VALIDATION.json').write_text(json.dumps(result, indent=2)+'\n')
    print(f'Frozen oracle report; {unchanged} previous experiment artifacts unchanged.')


if __name__ == '__main__':
    main()
