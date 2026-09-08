from pathlib import Path
import json

RUN = Path(__file__).resolve().parent
rows = []
records = 0
for path in sorted((RUN / 'coverage_results').glob('*.coverage.jsonl')):
    name = path.name.removesuffix('.coverage.jsonl')
    filename = f'{name}_vs_public_router.json'
    current = json.loads((RUN / 'coverage_results' / filename).read_text())
    previous = json.loads((RUN / 'discovery' / filename).read_text())
    assert current['games'] == previous['games']
    records += len(current['games'])
    coverage = [json.loads(line) for line in path.read_text().splitlines()]
    row = {'name': name, 'games': len(coverage), 'active_games': sum(bool(d['days']) for d in coverage),
        'days': sum(d['days'].bit_count() for d in coverage), 'hours': sum(sum(d['hours']) for d in coverage),
        'reversals': sum(len(d['reversals']) for d in coverage)}
    rows.append(row)
    print(row)
(RUN / 'COVERAGE.json').write_text(json.dumps({'full_records_exact': records, 'rows': rows,
    'reversal_fields': ['step', 'worker', 'x', 'y', 'wheat', 'shed_wheat', 'operation', 'previous_operation'],
    'scope': 'Immediate movement reversals with unchanged positive wheat. Exact repeated records establish unchanged instrumented behavior; reversals require individual causal interpretation.'}, indent=2) + '\n')
