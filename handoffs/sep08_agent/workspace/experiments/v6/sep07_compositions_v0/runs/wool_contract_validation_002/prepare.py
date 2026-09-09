"""Freeze the combined execution repair before a new broad seed panel."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
OLD = EXP / 'runs/rival_wool_validation_003'
candidate = 'wool_contract_repair_v2'
spec = json.loads((OLD / 'FRESH_PREREGISTERED.json').read_text())
spec.update(created_utc=datetime.now(timezone.utc).isoformat(), candidate=candidate,
            baseline='rival_wool_context_v3', seed_start=1970000, seeds=2048)
spec['opponents'] = sorted(set(spec['opponents'] + ['rival_wool_context_v3', 'rival_wool_purchase_repair_v1']))
spec['scope'] = 'Combined sheep-purchase and one-wheat day-contract repair. Discovery3456 profiled games: only V5/2 changes; no paired margin loss. All discovery pools remain exposed. New1970000..1972047 both seats; no reuse of prior fresh seeds.'
spec['extra_requirement'] = 'Preserve preceding strict promotion gates: positive current-group utility confidence lower bound, no individual utility or mean regression, nonnegative every paired direct-parent margin. A cash-only gain does not suffice for promotion.'
files = list((EXP / 'runs/wool_contract_repair_002/proposals' / candidate).rglob('*'))
files += list((EXP / 'runs/wool_contract_repair_002/source').rglob('*'))
spec['candidate_files_sha256'] = {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files if p.is_file()}
target = RUN / 'FRESH_PREREGISTERED.json'
assert not target.exists()
target.write_text(json.dumps(spec, indent=2) + '\n')
for name in ['run_fresh.py', 'analyze_fresh.py']:
    text = (OLD / name).read_text().replace('19501907', '19701907')
    if name == 'analyze_fresh.py':
        start = text.index("          'limitations': [")
        end = text.index("\n(RUN / 'FRESH_ANALYSIS", start)
        text = text[:start] + "          'limitations': [spec['scope'], 'Specific known wool contracts; not arbitrary runtime replanning.', 'Fresh numeric success still requires native, causal, operational and frozen-build audits.']}" + text[end:]
    (RUN / name).write_text(text)
print('Frozen', candidate, 'against', len(spec['opponents']), 'opponents;', spec['seeds'], 'new seeds both seats.')
