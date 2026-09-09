from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
names = ['FRESH_PREREGISTERED', 'FRESH_ANALYSIS', 'NATIVE_ANALYSIS', 'CHECKS', 'FINAL_AUDITS', 'TRACE_ANALYSIS']
data = {name: json.loads((RUN / f'{name}.json').read_text()) for name in names}
spec = data['FRESH_PREREGISTERED']
candidate, parent = spec['candidate'], spec['baseline']
for path, expected in spec['candidate_files_sha256'].items():
    assert hashlib.sha256((EXP / path).read_bytes()).hexdigest() == expected, path
assert not data['FRESH_ANALYSIS']['gates']['direct_parent_paired_margin_nonnegative_every_game']
assert not data['NATIVE_ANALYSIS']['gates']['all_parent_paired_margins_nonnegative']
assert data['CHECKS']['generic_pair_debug_thread_all_records_equal']
assert data['FINAL_AUDITS']['frozen_full_records_equal']
regressions = {}
for stream in ['fresh', 'native']:
    own = json.loads((RUN / f'{stream}/{candidate}_vs_{parent}.json').read_text())['games']
    baseline = json.loads((RUN / f'{stream}/{parent}_vs_{parent}.json').read_text())['games']
    rows = []
    for a, b in zip(own, baseline):
        assert (a['seed'], a['seat']) == (b['seed'], b['seat'])
        gain = a['cash'] - a['opponent_cash'] - b['cash'] + b['opponent_cash']
        if gain < 0:
            rows.append({'seed': a['seed'], 'seat': a['seat'], 'margin_gain': gain,
                         'own_cash_gain': a['cash'] - b['cash'], 'rival_cash_gain': a['opponent_cash'] - b['opponent_cash']})
    regressions[stream] = rows
now = datetime.now(timezone.utc).isoformat()
report = {'completed_utc': now, 'candidate': candidate, 'baseline': parent, 'promoted': False,
          'decision': 'Rejected under the preregistered per-game direct-parent margin gate in both fresh and native panels. Aggregate league gains and operational checks do not override that failure.',
          'fresh_games': data['FRESH_ANALYSIS']['games'], 'native_pass_games': data['NATIVE_ANALYSIS']['games'],
          'operational_games': data['CHECKS']['games'], 'frozen_games': data['FINAL_AUDITS']['frozen_games'],
          'groups': data['FRESH_ANALYSIS']['groups'], 'fresh_parent': data['FRESH_ANALYSIS']['metrics'][candidate][parent],
          'fresh_gates': data['FRESH_ANALYSIS']['gates'], 'native_gates': data['NATIVE_ANALYSIS']['gates'],
          'parent_paired_regressions': regressions,
          'causal_witness': data['TRACE_ANALYSIS']['finding'],
          'causal_scope': 'The floor-price mechanism is proved for native seed2004097 seat0 only; the other failed fresh cases are listed for later diagnosis.',
          'all_frozen_source_hashes_unchanged': True, 'source_files': len(spec['candidate_files_sha256']),
          'files_sha256': {str((RUN / f'{name}.json').relative_to(EXP)): hashlib.sha256((RUN / f'{name}.json').read_bytes()).hexdigest() for name in names},
          'next': data['TRACE_ANALYSIS']['next_hypothesis']}
target = EXP / 'results/empty_sale_slots_m2_validation.json'
target.write_text(json.dumps(report, indent=2) + '\n')
text = '# Empty-sale removal validation\n\n' + report['decision'] + '\n\n'
text += f"{report['fresh_games']:,} fresh games, {report['native_pass_games']:,} native/PASS games, {report['operational_games']:,} operational games, and {report['frozen_games']} rebuilt full records. All frozen source hashes unchanged.\n\n"
text += f"Direct parent: {report['fresh_parent']['wins']} wins, {report['fresh_parent']['ties']} ties, {report['fresh_parent']['losses']} losses; mean margin {report['fresh_parent']['mean_margin']:+.2f}.\n\n"
text += '| Panel | Parent utility | Candidate utility | Gain 95% interval (percentage points) |\n| --- | ---: | ---: | ---: |\n'
for label, group in report['groups'].items():
    text += f"| {label} | {group['utilities'][parent]*100:.3f}% | {group['utilities'][candidate]*100:.3f}% | {group['gain_95pct'][0]*100:+.3f} to {group['gain_95pct'][1]*100:+.3f} |\n"
text += '\nPaired parent regressions:\n\n'
for stream, rows in regressions.items():
    text += f'- {stream}: {len(rows)} cases, minimum margin gain {min(r["margin_gain"] for r in rows):+d}. Exact seeds and cash changes are in the central report.\n'
text += '\n' + report['causal_witness'] + '\n\n' + report['causal_scope'] + '\n\n'
text += 'Reproduce in order: prepare.py, run_fresh.py, analyze_fresh.py, run_native.py, analyze_native.py, run_checks.py, check_frozen.py, run_trace.py, analyze_trace.py, finalize.py. Output folders intentionally fail if they exist; preserve existing evidence and use a clean copied run with adjusted output paths for reruns. Read FRESH_PREREGISTERED.json before testing. The isolated frozen/FROZEN.json contains its exact rebuild command.\n\n'
text += report['next'] + '\n'
(RUN / 'RESULTS.md').write_text(text)
(RUN / 'README.md').write_text('# Frozen empty-sale candidate audit\n\nRead RESULTS.md and ../../results/empty_sale_slots_m2_validation.json. Candidate empty_sale_slots_m2 is operational and reproducible but rejected by its preregistered parent regression gates. Sources remain frozen; strongest reference is unchanged. TRACE_ANALYSIS.json explains the native floor-price failure. No Kaggle submission.\n')
entry = f'\n{now}: empty_sale_slots_m2 rejected:67584 fresh/5632 native-PASS/1024 operational/64 isolated rebuilt games. Current league93.956%->94.725%, direct906W38T80L,+239.62margin; both preregistered parent per-game gates fail. Fresh regressions{len(regressions["fresh"])} and native{len(regressions["native"])}. Exact native trace proves strawberry floor-price inventory effect, not input movement; same-state immediate+2 leads eventual-6margin. Floor guards are a new discovery study, no promotion.\n'
for name in ['LINEAGE.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (EXP / name).open('a') as out:
        out.write(entry)
print(report['decision'])
print('Regressions:', regressions)
print('Frozen source files unchanged:', report['source_files'])
