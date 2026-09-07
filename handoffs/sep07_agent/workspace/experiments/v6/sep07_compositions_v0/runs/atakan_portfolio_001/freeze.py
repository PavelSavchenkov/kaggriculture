"""Assert retained policy parity with frozen builds and hash final artifacts."""
import hashlib
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent
ROOT = RUN.parents[4]
RETAINED = ['atakan_demand', 'atakan_value_margin']


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    sources = [RUN / 'source' / p for p in ['agent.cpp', 'agent.hpp', 'data.inc']]
    for name in RETAINED:
        sources += sorted((RUN / 'proposals' / name / 'source').glob('*'))
    manifest = json.loads((RUN / 'build/generic/BUILD.json').read_text())
    for path in sources:
        assert digest(path) == manifest['source_sha256'][str(path.relative_to(ROOT))]
    report = json.loads((RUN / 'REPORT.json').read_text())
    attribution = json.loads((RUN / 'CASH_ATTRIBUTION.json').read_text())
    assert attribution['exact_game_parity'] == 960
    assert report['routing_matches_complete_fixed_branch_records'] == 1280
    report['product_attribution'] = 'CASH_ATTRIBUTION.json'
    report['seed1028_attribution'] = 'cash_case_1028_v5.json'
    (RUN / 'REPORT.json').write_text(json.dumps(report, indent=2) + '\n')
    for name in RETAINED:
        path = RUN / 'proposals' / name / 'IMPORT.json'
        metadata = json.loads(path.read_text())
        metadata['policy_source_sha256'] = {str(p.relative_to(RUN)): digest(p) for p in sources if 'proposals' not in p.parts or name in p.parts}
        metadata['product_attribution'] = 'runs/atakan_portfolio_001/CASH_ATTRIBUTION.json'
        path.write_text(json.dumps(metadata, indent=2) + '\n')
    artifacts = sorted(p for p in RUN.rglob('*') if p.is_file() and 'source_snapshot' not in p.parts
                       and '__pycache__' not in p.parts and p.name != 'FINAL_VALIDATION.json'
                       and (p.suffix in {'.json', '.csv', '.cpp', '.hpp', '.py', '.md', '.inc'}))
    result = {'retained_packages': [str((RUN / 'proposals' / name).relative_to(ROOT)) for name in RETAINED],
              'policy_matches_frozen_build': True, 'discovery_only': True,
              'decision': 'Operational league specialists; neither supersedes the current main agent. Parent registers them.',
              'artifact_sha256': {str(p.relative_to(RUN)): digest(p) for p in artifacts}}
    (RUN / 'FINAL_VALIDATION.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps({k:v for k,v in result.items() if k != 'artifact_sha256'}, indent=2))


if __name__ == '__main__':
    main()
