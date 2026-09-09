from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
SOURCE = EXP / 'runs/late_goose_context_001'
OUT = EXP / 'runs/late_portfolio_validation_001'
OUT.mkdir(exist_ok=False)
for name in ['run_fresh.py', 'analyze_fresh.py']:
    original = (SOURCE / name).read_text()
    code = original.replace('late_goose_wheat_context', 'late_value_s32_t0_r05')
    code = code.replace('1810000', '1830000').replace('1810512', '1830512').replace('18101907', '18301907')
    code = code.replace('runs/late_animal_schedule_001/proposals/late_value_s32_t0_r05', 'runs/late_portfolio_001/proposals/late_value_s32_t0_r05')
    code = code.replace("old_spec['opponents'] + ['opening_q32_b13_v1']", "old_spec['opponents'] + ['opening_q32_b13_v1', 'late_goose_wheat_context']")
    code = code.replace('Preserve parent tomato/wheat context using its existing day12 rule; previous1790000 seeds are now diagnostic. New1830000 seeds are unused validation.',
                        'Four-way estimated value minus half a sampled standard deviation, selected from42 diagnostic parameter settings on1790000;1830000 seeds are unused validation.')
    if name == 'analyze_fresh.py':
        code = code.replace("prefix = json.loads((EXP / 'runs/late_animal_schedule_001/PACKAGE_PREFIX_PARITY.json').read_text())", "prefix = {'full_records_equal': json.loads((EXP / 'runs/late_portfolio_001/SELECTOR_PARITY.json').read_text())['full_records_equal']}")
        code = code.replace("off = json.loads((EXP / 'runs/late_animal_schedule_001/PACKAGE_PARITY.json').read_text())[0]", "off = {'full_records_equal': all(row['full_records_equal'] for row in json.loads((EXP / 'runs/late_portfolio_001/INITIAL_PARITY.json').read_text()))}")
    (OUT / name).write_text(code)
(OUT / 'SCRIPT_LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'source': str(SOURCE.relative_to(EXP)), 'source_sha256': {name: hashlib.sha256((SOURCE / name).read_bytes()).hexdigest() for name in ['run_fresh.py', 'analyze_fresh.py']},
    'changes': ['candidate four-way selector', 'unused1830000 seeds', 'add previous goose context opponent', 'new exact selector parity requirement']}, indent=2) + '\n')
print(OUT)
