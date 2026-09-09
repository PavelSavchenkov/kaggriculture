"""Check the frozen candidate's preregistered gates and promote the local reference."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import math
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
CANDIDATE, BASELINE = 'wool_family_context_v2', 'late_value_s32_t0_r05'


def read(path):
    return json.loads(path.read_text())


def metrics(games):
    margins = [g['cash'] - g['opponent_cash'] for g in games]
    cash = sorted(g['cash'] for g in games)
    tail = math.ceil(len(games) / 10)
    return {'games': len(games), 'wins': sum(m > 0 for m in margins), 'ties': sum(m == 0 for m in margins),
            'utility': statistics.mean(1 if m > 0 else .5 if m == 0 else 0 for m in margins),
            'mean_margin': statistics.mean(margins), 'mean_cash': statistics.mean(cash),
            'cvar10_margin': statistics.mean(sorted(margins)[:tail]),
            'pass_J': .8 * statistics.mean(cash) + .2 * statistics.mean(cash[:tail])}


def contrast(a, b):
    assert [(g['seed'], g['seat']) for g in a] == [(g['seed'], g['seat']) for g in b]
    ma, mb = metrics(a), metrics(b)
    row = {key + '_gain': ma[key] - mb[key] for key in ['utility', 'mean_margin', 'mean_cash', 'cvar10_margin', 'pass_J']}
    for key in ['produced', 'sold', 'discarded']:
        row[key + '_gain'] = [statistics.mean(x[key][i] - y[key][i] for x, y in zip(a, b)) for i in range(12)]
    row['buys_gain'] = [statistics.mean(x['profile']['buys'][i] - y['profile']['buys'][i] for x, y in zip(a, b)) for i in range(12)]
    for key in ['unit_faults', 'worker_days', 'opponent_cash']:
        row[key + '_gain'] = statistics.mean(x[key] - y[key] for x, y in zip(a, b))
    for key in ['hires', 'hire_cost', 'land_cost']:
        row[key + '_gain'] = statistics.mean(x['profile'][key] - y['profile'][key] for x, y in zip(a, b))
    row['changed_own_actions'] = sum(x['action_hash'] != y['action_hash'] for x, y in zip(a, b))
    row['changed_rival_actions'] = sum(x['opponent_action_hash'] != y['opponent_action_hash'] for x, y in zip(a, b))
    row['changed_production'] = sum(x['produced'] != y['produced'] for x, y in zip(a, b))
    return row


def main():
    fresh, checks, final = [read(RUN / name) for name in ['FRESH_ANALYSIS.json', 'CHECKS.json', 'FINAL_AUDITS.json']]
    frozen, spec = read(RUN / 'frozen/FROZEN.json'), read(RUN / 'FRESH_PREREGISTERED.json')
    assert all(fresh['gates'].values()) and checks['generic_pair_debug_thread_all_records_equal'] and final['frozen_full_records_equal']
    for root, hashes in [(ROOT, frozen['files_sha256']), (EXP, spec['candidate_files_sha256'])]:
        assert all(hashlib.sha256((root / path).read_bytes()).hexdigest() == value for path, value in hashes.items())
    selector = read(RUN / 'SELECTOR_PARITY.json')
    assert selector['all_game_fields_equal'] and selector['games'] == 8192
    native = {}
    for opponent in [BASELINE, 'investment_context_guarded_001_best', 'teammate_shoprouter', 'king_rc4',
                     'public_router', 'public_router_v5', 'public_router_v52', 'junghoon_wool_sales', 'pass']:
        games = {a: read(RUN / f'native/{a}_vs_{opponent}.json')['games'] for a in [CANDIDATE, BASELINE]}
        native[opponent] = {'metrics': {a: metrics(g) for a, g in games.items()}, 'contrast': contrast(games[CANDIDATE], games[BASELINE])}
    limits = read(RUN / 'NATIVE_PLAN.json')['gates']
    assert native[BASELINE]['metrics'][CANDIDATE]['utility'] > .5 and native[BASELINE]['metrics'][CANDIDATE]['mean_margin'] > 0
    assert all(row['contrast']['mean_margin_gain'] >= limits['minimum_mean_margin_gain'] and
               row['contrast']['utility_gain'] >= limits['minimum_individual_utility_gain'] for row in native.values())
    assert native['pass']['contrast']['pass_J_gain'] >= limits['minimum_pass_J_gain']
    now = datetime.now(timezone.utc).isoformat()
    report = {'promoted_utc': now, 'candidate': CANDIDATE, 'parent': BASELINE,
              'status': 'Promoted experimental reference after fresh numeric, native, causal, operational, selector and frozen-build audits.',
              'fresh': fresh, 'native_and_pass': native, 'operational': checks, 'selector_parity': selector,
              'frozen': {'path': 'runs/wool_family_validation_002/frozen/FROZEN.json', 'dependencies': len(frozen['files_sha256']),
                         'full_native_records_exact': final['frozen_games']},
              'lineage': 'runs/wool_family_context_v2_001/LINEAGE.json',
              'causal_scope': 'All4608 native/PASS profiles; the whole-farm choice changes animal and crop output, trades, labor and occasionally rival actions. It is not a pure labor intervention. Separate solver attribution is in runs/v52_family_001/OPTIMIZED_CAUSAL.json.',
              'tradeoffs': ['Fresh teammate loses1win/1024 and about$311 worst-decile margin while mean improves$62.10.',
                            'Native public_router loses2wins/256 and submitted investment loses1/256; all paired native mean margins improve.',
                            'Direct v1 specialist still beats v2: v2 has80W816T128L and mean-$328.70. v1 fails broad promotion gates and is retained in the league.',
                            'This is a broad league improvement, not pairwise dominance over every previous version.'],
              'limitations': ['Entry uses empirical observed-shop contexts learned from complete counterfactual courses.',
                              'General raw-composition valuation, greedy placement and independent cold construction remain incomplete.',
                              'Most donor calendar days depend on exact observed-state guards; mismatches retain source schedules.',
                              'Top-player complete controllers are not all available; no leaderboard rank inferred.',
                              'No new official agent copy, Git operation or Kaggle submission.']}
    (EXP / 'results/wool_family_context_v2_validation.json').write_text(json.dumps(report, indent=2) + '\n')
    current = read(EXP / 'CURRENT_REFERENCE.json')
    assert current['name'] == BASELINE
    (RUN / 'PREVIOUS_REFERENCE.json').write_text(json.dumps(current, indent=2) + '\n')
    current.update(name=CANDIDATE, path='runs/wool_family_context_v2_001/proposals/' + CANDIDATE,
                   promoted_utc=now, report='results/wool_family_context_v2_validation.json', previous=BASELINE)
    (EXP / 'CURRENT_REFERENCE.json').write_text(json.dumps(current, indent=2) + '\n')
    package = EXP / current['path']
    lineage = read(package / 'IMPORT.json')
    lineage.update(status=report['status'], promoted_utc=now, validation=current['report'])
    (package / 'IMPORT.json').write_text(json.dumps(lineage, indent=2) + '\n')
    readme = package / 'README.md'
    readme.write_text(readme.read_text().replace('Experimental, not promoted.', 'Promoted local reference; validation and tradeoffs are in IMPORT.json.'))
    readme = EXP / 'README.md'
    text = readme.read_text()
    start, end = text.index('Current broad search starting point'), text.index('Previous reference (18:42 UTC):')
    text = text[:start] + f'''Current broad search starting point ({now[11:16]} UTC):
`{current['path']}/`.
Select a complete sheep-heavy V5/2 continuation on day6 from the first two
observed shops, with checked stock restoration and17 improved day schedules.
Otherwise retain the prior adaptive crop/animal portfolio.
Read `CURRENT_REFERENCE.json` and `results/wool_family_context_v2_validation.json`.
Fresh51200 games across25opponents pass all registered gates. Direct parent
139W818T67L, mean+$363.36; V5/2 win rate78.32%, teammate97.75%.
All paired mean margins improve. Small win/tail losses and the direct loss to
the unpromoted v1 specialist remain explicit. Native, causal, operational,
selector and227-source rebuilt checks pass.

Previous reference (20:22 UTC):
`runs/late_portfolio_001/proposals/late_value_s32_t0_r05/`;
`results/late_portfolio_validation.json` retains its evidence.

''' + text[end:]
    readme.write_text(text)
    entry = f'\n{now}: Promoted {CANDIDATE}. Whole sheep-heavy continuation with observed-shop selection and compiled labor savings. Fresh51200games/25opponents passes all gates; direct parent139W818T67L,+$363.36. Native4608profiles,896 operational,8192 selector parity,227 frozen inputs and256 rebuilt full records pass. All paired mean margins improve; teammate-1freshwin/1024 and v1 specialist direct loss retained explicitly. No submission or Git.\n'
    for filename in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md', 'OBJECTIVE_COVERAGE.md']:
        with (EXP / filename).open('a') as out:
            out.write(entry)
    print('Promoted', CANDIDATE, now)
    for opponent, row in native.items():
        print(opponent, row['contrast'])


if __name__ == '__main__':
    main()
