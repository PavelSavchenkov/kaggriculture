from datetime import datetime, timezone
from pathlib import Path
import copy
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
TEMPLATE = EXP / 'runs/empty_sale_validation_sep08_001'
SOURCE = EXP / 'runs/empty_sale_floor_sep08_001'
candidate, baseline = 'empty_sale_floor_m1', 'observed_sale_lead_start_216'
assert not (RUN / 'FRESH_PREREGISTERED.json').exists()
discovery = json.loads((SOURCE / 'ANALYSIS.json').read_text())
assert discovery['games'] == 4096 and discovery['old_empty_removal_exact_full_records'] == 1024
for row in discovery['rows']:
    selected = row['modes']['1']
    assert selected['parent_margin_gain'] >= 0 and selected['parent_utility_gain'] >= 0
    assert selected['parent_minimum_paired_margin_gain'] >= 0
witness = json.loads((SOURCE / 'WITNESS_ANALYSIS.json').read_text())
assert witness['native_complete_old_controls'] == 512 and witness['custom_endpoint_controls'] == 4
for label in ['native', 'custom']:
    assert witness['rows'][f'{label}_{candidate}']['minimum_parent_paired_margin'] >= 0
previous = json.loads((TEMPLATE / 'FRESH_PREREGISTERED.json').read_text())
opponents = sorted(set(previous['opponents'] + ['empty_sale_slots_m2']))
grouping = copy.deepcopy(previous['grouping'])
grouping['prior_versions'].append('empty_sale_slots_m2')
hashes = dict(previous['candidate_files_sha256'])
for relative, expected in hashes.items():
    assert hashlib.sha256((EXP / relative).read_bytes()).hexdigest() == expected, relative
for path in (SOURCE / f'proposals/{candidate}').rglob('*'):
    if path.is_file():
        hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
spec = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': candidate, 'baseline': baseline,
    'seed_start': 2100000, 'seeds': 512, 'seat_mode': 'both', 'opponents': opponents,
    'grouping': grouping, 'historical_grouping': previous['historical_grouping'],
    'targeted_response_gates': previous['targeted_response_gates'], 'candidate_files_sha256': hashes,
    'inherited_documentation_amendment': previous['inherited_documentation_amendment'],
    'scope': 'Frozen price-floor guard mode1. Before creating this file, rg found no2100000/2103000/2104000 in experiment Python or preregistered/protocol files. Final900000 remains unused. Old failed candidate is included as an additional league opponent.',
    'discovery': 'runs/empty_sale_floor_sep08_001/ANALYSIS.json',
    'causal_witness': 'runs/empty_sale_floor_sep08_001/WITNESS_ANALYSIS.json',
    'selection_reason': 'Mode1 repairs both exposed regressions, preserves all eight discovery opponent utilities and paired margins, and retains substantially more mean-margin gain than the matched-volume or100-unit guards. No fitting on this fresh panel.',
    'native_audit': {'seed_start': 2104000, 'seeds': 128, 'seat_mode': 'both',
        'minimum_individual_utility_gain': 0, 'minimum_mean_margin_gain': 0,
        'direct_parent_paired_margin_nonnegative_every_game': True, 'minimum_pass_J_gain': 0},
    'operational': 'Generic/pair/debug and threads1/4 exact complete records; self/PASS; active changes in custom/native streams; isolated per-instance state and frozen rebuild.'}
(RUN / 'FRESH_PREREGISTERED.json').write_text(json.dumps(spec, indent=2) + '\n')
for name in ['run_fresh.py', 'analyze_fresh.py', 'run_native.py', 'analyze_native.py', 'run_checks.py', 'check_frozen.py']:
    text = (TEMPLATE / name).read_text()
    text = text.replace('empty_sale_slots_m2', candidate).replace('2003000', '2103000').replace('2004000', '2104000').replace('20000107', '21000107')
    text = text.replace('removal counter are per-instance', 'removal/protection counters are per-instance')
    text = text.replace('removes only non-input zero SELLs after step216; no seed, future randomness or rival-private stock.', 'removes non-input zero SELLs after step216 only when later requested sales stay above price1 under observed market stock and our requested trade volumes; no seed, future randomness or rival-private stock.')
    if name == 'analyze_fresh.py':
        text = text.replace('Native, operational, causal and isolated frozen-build audits remain required.', 'The guard uses own requested volume; arbitrary simultaneous rival volume is not bounded. Native, operational and isolated frozen-build audits remain required.')
    (RUN / name).write_text(text)
(RUN / 'README.md').write_text('# Frozen floor-guard validation\n\nRead FRESH_PREREGISTERED.json before running. Candidate empty_sale_floor_m1 and accepted parent are frozen for69632 fresh games across34 opponents; additional native/PASS, operational and isolated rebuild checks remain required. Mode1 was selected from exposed discovery and causal witnesses before any new-seed outcome. No promotion or submission yet.\n')
print('Frozen', candidate, 'against', len(opponents), 'opponents;', 2*1024*len(opponents), 'fresh games.')
