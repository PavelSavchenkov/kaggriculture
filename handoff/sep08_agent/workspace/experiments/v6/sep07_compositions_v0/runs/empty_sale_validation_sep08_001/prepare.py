"""Freeze validation before reading any new-seed results."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
TEMPLATE = EXP / 'runs/observed_sale_validation_002'
SOURCE = EXP / 'runs/empty_sale_slots_sep08_001'
candidate, baseline = 'empty_sale_slots_m2', 'observed_sale_lead_start_216'
assert not (RUN / 'FRESH_PREREGISTERED.json').exists()
discovery = json.loads((SOURCE / 'ANALYSIS.json').read_text())
assert discovery['screen_passed'] and discovery['old_control_records_exact'] == 1024
assert discovery['m1_m2_records_exact'] == 1024
previous = json.loads((TEMPLATE / 'FRESH_PREREGISTERED.json').read_text())
opponents = sorted(set(previous['opponents'] + [baseline]))
grouping = previous['grouping']
assert baseline not in grouping['prior_versions']
grouping['prior_versions'].append(baseline)
hashes = dict(previous['candidate_files_sha256'])
amendment = json.loads((EXP / 'results/observed_sale_lead_start216_validation.json').read_text())['documentation_amendment']
assert amendment['path'].endswith('/README.md') and amendment['fresh_binary_policy_unchanged']
assert hashes[amendment['path']] == amendment['before_sha256']
hashes[amendment['path']] = amendment['after_sha256']
for relative, expected in hashes.items():
    assert hashlib.sha256((EXP / relative).read_bytes()).hexdigest() == expected
for path in (SOURCE / f'proposals/{candidate}').rglob('*'):
    if path.is_file():
        hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
spec = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': candidate, 'baseline': baseline,
    'seed_start': 2000000, 'seeds': 512, 'seat_mode': 'both', 'opponents': opponents,
    'grouping': grouping, 'historical_grouping': previous['historical_grouping'],
    'targeted_response_gates': previous['targeted_response_gates'], 'candidate_files_sha256': hashes,
    'inherited_documentation_amendment': amendment,
    'scope': 'Frozen late non-input empty-sale removal. No2000000/2003000/2004000 seeds found in prior experiment Python or preregistered/protocol files before reserving these pools. Final900000 remains unused.',
    'discovery': 'runs/empty_sale_slots_sep08_001/ANALYSIS.json',
    'selection_reason': 'Modes1/2 produce identical1024 full records. Select the observed late non-input change; keep inherited early funding and input-order behavior. No fitting on this fresh panel.',
    'native_audit': {'seed_start': 2004000, 'seeds': 128, 'seat_mode': 'both', 'minimum_individual_utility_gain': 0, 'minimum_mean_margin_gain': 0, 'direct_parent_paired_margin_nonnegative_every_game': True, 'minimum_pass_J_gain': 0},
    'operational': 'Generic/pair/debug and threads1/4 exact complete records; full self/PASS; active changes in both custom/native streams; immutable parent tables and isolated per-instance state.'}
(RUN / 'FRESH_PREREGISTERED.json').write_text(json.dumps(spec, indent=2) + '\n')


def adapt(text):
    return (text.replace('observed_sale_lead_start_216', '__NEW_CANDIDATE__')
        .replace('rival_wool_context_v3', baseline).replace('__NEW_CANDIDATE__', candidate)
        .replace('1993000', '2003000').replace('1994000', '2004000').replace('19901907', '20000107'))


for name in ['run_fresh.py', 'analyze_fresh.py', 'run_native.py', 'analyze_native.py', 'run_checks.py', 'check_frozen.py']:
    text = adapt((TEMPLATE / name).read_text())
    text = text.replace("Path(json.loads((EXP / 'runs/observed_sale_lead_004/BUILD.json').read_text())['binary'])", "EXP.parents[2] / json.loads((RUN / 'FRESH_BUILD.json').read_text())['binary']")
    text = text.replace('sale_lead_changed_games', 'empty_slot_changed_games')
    text = text.replace('Checks must exercise the sale lead, not only its unchanged parent.', 'Checks must exercise empty-sale removal, not only its unchanged parent.')
    text = text.replace('One policy and forecast/suppression state per instance. SharedVector calendars are immutable after construction; no shared mutable episode state.', 'Inherited policy/forecast/suppression state and removal counter are per-instance. Parent calendars remain immutable; no shared mutable episode state.')
    text = text.replace('Only current public/own observation and independently copied policy state. Advance observed time for a one-turn sale forecast after step216; no seed, future randomness or rival-private stock.', 'Inherited observation-only policy. New transform reads current step and its own returned orders, removes only non-input zero SELLs after step216; no seed, future randomness or rival-private stock.')
    if name == 'analyze_fresh.py':
        start = text.index("          'limitations': [")
        end = text.index("\n(RUN / 'FRESH_ANALYSIS.json')", start)
        text = text[:start] + "          'limitations': ['Empty-order removal changes later market positions; it is not assumed economically neutral.', 'Discovery preserves own production, but shared-price changes may alter rival financing and later behavior.', 'Native, operational, causal and isolated frozen-build audits remain required.']}" + text[end:]
    (RUN / name).write_text(text)
print('Frozen', candidate, 'against', len(opponents), 'opponents;', 2 * 1024 * len(opponents), 'fresh games.')
