"""Freeze the retained specialist and verify predecessor artifacts stay unchanged."""
import hashlib
import json
from pathlib import Path

from prepare import NAMES

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
RETAINED = 'atakan_integrated_s64_margin'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    unchanged = {}
    for name in ['atakan_portfolio_001', 'atakan_oracle_ablation_001']:
        previous = RUN.parent / name
        manifest = json.loads((previous / 'FINAL_VALIDATION.json').read_text())
        for relative, expected in manifest['artifact_sha256'].items():
            assert digest(previous / relative) == expected, (name, relative)
        unchanged[name] = len(manifest['artifact_sha256'])
    policy = [RUN / 'source' / p for p in ['agent.cpp','agent.hpp','data.inc']]
    for name in NAMES:
        policy += sorted((RUN / 'proposals' / name / 'source').glob('*'))
    for kind in ['generic','debug','masked']:
        manifest = json.loads((RUN / 'build' / kind / 'BUILD.json').read_text())
        for path in policy:
            assert manifest['source_sha256'][str(path.relative_to(ROOT))] == digest(path)
    source_hashes = {str(p.relative_to(RUN)): digest(p) for p in policy}
    for name in NAMES:
        target = RUN / 'proposals' / name
        path = target / 'IMPORT.json';metadata = json.loads(path.read_text())
        metadata['status'] = 'Retained improved margin league specialist; not incumbent' if name == RETAINED else 'Archived experimental control'
        metadata['validation'] = str((RUN / 'REPORT.json').relative_to(EXP))
        metadata['policy_source_sha256'] = {p:h for p,h in source_hashes.items() if p.startswith('source/') or p.startswith(f'proposals/{name}/')}
        path.write_text(json.dumps(metadata, indent=2)+'\n')
        (target / 'README.md').write_text(f'# {name}\n\n{metadata["status"]}. Unknown-shop integration mode{metadata["sample_count"]}; objective {metadata["objective"]}. '
                                        'The fixed Atakan courses and all other forecast assumptions are unchanged. See ../../README.md and IMPORT.json for lineage and validation.\n')
    report_path = RUN / 'REPORT.json';report = json.loads(report_path.read_text())
    report['decision'] = 'Retain sampled64 margin as improved league specialist; keep existing demand specialist; archive full mean and other controls. No incumbent promotion or submission.'
    report['retained_package'] = str((RUN / 'proposals' / RETAINED).relative_to(ROOT))
    report_path.write_text(json.dumps(report, indent=2)+'\n')
    files = sorted(p for p in RUN.rglob('*') if p.is_file() and 'source_snapshot' not in p.parts and '__pycache__' not in p.parts and p.name != 'FINAL_VALIDATION.json')
    result = {'retained_package': report['retained_package'], 'policy_matches_all_frozen_builds': True,
              'unchanged_predecessor_artifacts': unchanged, 'policy_source_sha256': source_hashes,
              'commands': [['conda','run','-n','kaggriculture','python',str(RUN / script)] for script in ['prepare.py','build.py','diagnose.py','verify.py','report.py','freeze.py']],
              'additional_commands': ['build/*/BUILD.json','DISCOVERY_COMMANDS.json','FRESH_COMMANDS.json','CHECKS_COMMANDS.json','NATIVE_COMMANDS.json'],
              'artifact_sha256': {str(p.relative_to(RUN)): digest(p) for p in files}}
    (RUN / 'FINAL_VALIDATION.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({'retained_package': result['retained_package'], 'unchanged_predecessors': unchanged,
                      'core_sha256': source_hashes['source/agent.cpp'], 'header_sha256': source_hashes['source/agent.hpp'],
                      'course_data_sha256': source_hashes['source/data.inc']}, indent=2))


if __name__ == '__main__':
    main()
