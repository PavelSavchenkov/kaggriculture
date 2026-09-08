"""Freeze a new population comparison; preserve the old rejected audit."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
old = EXP / 'runs/empty_sale_floor_validation_sep08_001'
source = json.loads((old / 'FRESH_PREREGISTERED.json').read_text())
for relative, digest in source['candidate_files_sha256'].items():
    assert hashlib.sha256((EXP / relative).read_bytes()).hexdigest() == digest, relative
frozen = json.loads((old / 'frozen/FROZEN.json').read_text())
files = frozen['files_sha256']
for relative, digest in files.items():
    assert hashlib.sha256((ROOT / relative).read_bytes()).hexdigest() == digest, relative
search = subprocess.run(['rg', '-n', '2200000|2204000', str(EXP), '--glob', '*.py',
    '--glob', '*PREREGISTERED*.json', '--glob', '*PROTOCOL*.json', '--glob', '!prepare.py'], capture_output=True, text=True)
assert search.returncode == 1, search.stdout + search.stderr
binary = ROOT / json.loads((old / 'FRESH_BUILD.json').read_text())['binary']
lineage = json.loads((EXP / 'research/post_submission_lineage.json').read_text())
grouping = dict(source['grouping'])
grouping['prior_versions'] = [lineage['last_submitted_agent'], *[r['candidate'] for r in lineage['accepted_descendants']]]
opponents = sorted(set(source['opponents'] + [source['candidate']]))
native = ['observed_sale_lead_start_216', 'empty_sale_floor_m1', 'investment_context_guarded_001_best',
          'teammate_shoprouter', 'king_rc4', 'public_router', 'public_router_v5', 'public_router_v52',
          'junghoon_wool_sales', 'john_131', 'ahmed_v23', 'pass']
spec = {
    'created_utc': datetime.now(timezone.utc).isoformat(),
    'candidate': 'empty_sale_slots_m2', 'baseline': 'empty_sale_floor_m1',
    'accepted_reference': 'observed_sale_lead_start_216',
    'reason': 'Optimize playing strength. The unguarded candidate lost the old zero-paired-regression gate on two $6 cases but beats the guarded candidate directly. This is a new independent audit with population criteria, not a retroactive change to old results.',
    'fresh': {'seed_start': 2200000, 'seeds': 512, 'opponents': opponents},
    'native': {'seed_start': 2204000, 'seeds': 128, 'opponents': native},
    'seat_mode': 'both', 'grouping': grouping, 'historical_grouping': source['historical_grouping'],
    'primary_scope': 'Equal weight for teammate, public controllers, top replay reconstructions and the submitted plus nine accepted versions. Other experimental siblings are reported but do not inflate primary group weight.',
    'gates': {
        'fresh_primary_utility_95pct_lower_min': -0.0025,
        'fresh_primary_mean_margin_95pct_lower_strictly_positive': True,
        'fresh_historical_utility_95pct_lower_min': -0.0025,
        'fresh_individual_primary_utility_gain_min': -0.0025,
        'fresh_individual_primary_mean_margin_gain_min': 0,
        'fresh_individual_primary_cvar10_margin_gain_min': -50,
        'direct_guard_utility_strictly_above': 0.5,
        'direct_guard_mean_margin_strictly_above': 0,
        'native_individual_utility_gain_min': -0.004,
        'native_individual_mean_margin_gain_min': 0,
        'native_individual_cvar10_margin_gain_min': -50,
        'native_pass_mean_cash_gain_min': 0,
        'native_pass_cvar10_cash_gain_min': -50,
    },
    'gate_rationale': 'Allow at most about two fresh or one native lost win per opponent for sampling noise, while requiring nonnegative mean margins, bounded worst-decile changes and positive primary mean-margin confidence. No per-game no-loss rule. Report every regression, including secondary opponents.',
    'bootstrap': {'seed': 22000107, 'resamples': 10000, 'unit': 'seed, retaining both seats and all opponents together'},
    'binary': str(binary.relative_to(ROOT)), 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
    'build': str((old / 'FRESH_BUILD.json').relative_to(EXP)),
    'candidate_files_sha256': source['candidate_files_sha256'],
    'frozen_root_files_sha256': files,
    'operational_evidence': [str(p.relative_to(EXP)) for p in [
        EXP / 'runs/empty_sale_validation_sep08_001/CHECKS.json',
        EXP / 'runs/empty_sale_validation_sep08_001/FINAL_AUDITS.json',
        old / 'CHECKS.json', old / 'FINAL_AUDITS.json']],
    'operational_reuse': 'Both unchanged policies already have 1024 operational and 64 isolated rebuilt games. Recheck hashes and reuse this evidence; new comparison does not alter code or packaging.',
    'selection': 'Require all gates before replacing accepted reference with unguarded candidate. If gates fail, retain both with results and leave accepted reference unchanged pending diagnosis. Final seed900000 untouched.',
}
for path in spec['operational_evidence']:
    spec.setdefault('evidence_sha256', {})[path] = hashlib.sha256((EXP / path).read_bytes()).hexdigest()
with (RUN / 'PREREGISTERED.json').open('x') as out:
    json.dump(spec, out, indent=2); out.write('\n')
print('Frozen new population comparison:', len(opponents)*2048, 'fresh,', len(native)*512, 'native/PASS games.')
