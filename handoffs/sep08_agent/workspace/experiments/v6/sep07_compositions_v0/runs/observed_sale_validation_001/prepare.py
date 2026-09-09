"""Freeze broad sale-lead validation after complete discovery and parity."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
SOURCE = EXP / 'runs/observed_sale_lead_003'
TEMPLATE = EXP / 'runs/rival_wool_validation_003'
candidate, baseline = 'observed_sale_lead_shared_v3', 'rival_wool_context_v3'
assert not (RUN / 'FRESH_PREREGISTERED.json').exists()
discovery = json.loads((SOURCE / 'DISCOVERY_ANALYSIS.json').read_text())
for opponent, variants in discovery['comparisons'].items():
    assert variants['sale_lead_shared_control']['full_records_equal']
    assert all(row.get('v2_full_records_equal', False) for row in variants.values())
    assert variants[candidate]['utility_gain'] >= 0
    assert variants[candidate]['mean_margin_gain'] >= 0
assert discovery['comparisons'][baseline][candidate]['paired_margin_min'] >= 0

previous = json.loads((TEMPLATE / 'FRESH_PREREGISTERED.json').read_text())
opponents = sorted(set(previous['opponents'] + [baseline, 'ahmed_v23', 'rival_wool_purchase_repair_v1', 'wool_contract_repair_v2']))
catalog = json.loads((EXP / 'configs/league.json').read_text())
names = set(catalog)
assert set(opponents) <= names, set(opponents) - names
grouping = previous['grouping']
grouping['prior_versions'] += ['opening_q32_b13_v1', 'late_value_s32_t0_r05', 'wool_family_context_v2', baseline]
grouping['public_controllers'].append('ahmed_v23')
paths = [p for folder in ['source', 'parent_source', f'proposals/{candidate}'] for p in (SOURCE / folder).rglob('*') if p.is_file()]
hashes = {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
spec = {
    'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': candidate, 'baseline': baseline,
    'seed_start': 1990000, 'seeds': 512, 'seat_mode': 'both', 'opponents': opponents,
    'grouping': grouping, 'historical_grouping': previous['historical_grouping'],
    'targeted_response_gates': previous['targeted_response_gates'],
    'scope': 'Frozen all-noninput sale lead selected after complete ten-opponent discovery. Earlier1990000 search found no recorded use; no policy fitting on this panel. Final900000 remains unused.',
    'discovery': 'runs/observed_sale_lead_003/DISCOVERY_ANALYSIS.json',
    'selection_reason': 'All-noninput mode improves more than milk/wool-only on exposed comparisons without a discovered utility or mean-margin regression. Require the same strict fresh gates as the current reference.',
    'candidate_files_sha256': hashes,
    'native_audit': {'seed_start': 1994000, 'seeds': 128, 'seat_mode': 'both', 'minimum_individual_utility_gain': 0, 'minimum_mean_margin_gain': 0, 'direct_parent_paired_margin_nonnegative_every_game': True, 'minimum_pass_J_gain': 0},
    'operational': 'Generic/pair/debug and threads1/4 exact full records; complete self/PASS; active sale changes in both custom and native streams; immutable calendars with isolated mutable state.',
}
(RUN / 'FRESH_PREREGISTERED.json').write_text(json.dumps(spec, indent=2) + '\n')

fresh = (TEMPLATE / 'run_fresh.py').read_text().replace("'--threads', '6'", "'--threads', '10'")
(RUN / 'run_fresh.py').write_text(fresh)
analysis = (TEMPLATE / 'analyze_fresh.py').read_text().replace('19501907', '19901907')
start = analysis.index("          'limitations': [")
end = analysis.index("\n(RUN / 'FRESH_ANALYSIS.json')", start)
analysis = analysis[:start] + "          'limitations': ['A one-turn forecast uses only current observation and copied policy state; all sampled diagnostic forecasts match, not a general future-state guarantee.', 'Production and labor are preserved on most discovery games; shared-price changes can alter rival behavior and subsequent execution.', 'Native, operational and isolated frozen-build audits remain separate requirements.']}" + analysis[end:]
(RUN / 'analyze_fresh.py').write_text(analysis)

checks = (TEMPLATE / 'run_checks.py').read_text().replace("'rival_wool_context_v3', 'wool_family_context_v2'", repr(candidate) + ', ' + repr(baseline))
checks = checks.replace("generic = EXP.parents[2] / json.loads((RUN / 'FRESH_BUILD.json').read_text())['binary']", "generic = Path(json.loads((EXP / 'runs/observed_sale_lead_003/BUILD.json').read_text())['binary'])")
checks = checks.replace('1913000', '1993000').replace('new delayed branch', 'new sale lead')
checks = checks.replace("'additional_branch_changed_games'", "'sale_lead_changed_games'")
checks = checks.replace('One base policy and selection state per instance. Shared calendars are immutable; no mutable global episode state.', 'One policy and forecast/suppression state per instance. Copied calendars are immutable SharedVectors; append after sharing aborts. No mutable global episode state.')
old = 'Day6 hour1 selector reads observed shops, public rival workforce and net product flows and public spending after hires and wheat, and own guarded prefix. Two policy instances each retain independent state. No seed or hidden state; the retained parent uses its existing later observed decisions.'
checks = checks.replace(old, 'Sale lead reads current own stock and unit actions, current market prices, and a time-advanced copy of the public/own observation. Forecast policy state is independently copied. No seed, future randomness or rival-private state.')
(RUN / 'run_checks.py').write_text(checks)
native = (TEMPLATE / 'run_native.py').read_text().replace("agents = ['rival_wool_context_v3', 'wool_family_context_v2']", f'agents = [{candidate!r}, {baseline!r}]')
native = native.replace("opponents = ['wool_family_context_v2',", f'opponents = [{baseline!r},').replace("'john_131']", "'john_131', 'ahmed_v23']").replace('1954000', '1994000')
(RUN / 'run_native.py').write_text(native)
print('Frozen', candidate, 'against', len(opponents), 'opponents;', 2 * 1024 * len(opponents), 'fresh games.')
