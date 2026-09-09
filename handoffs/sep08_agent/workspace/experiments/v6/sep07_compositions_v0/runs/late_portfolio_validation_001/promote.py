from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import math
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
candidate, baseline = 'late_value_s32_t0_r05', 'opening_q32_b13_v1'
confirm = json.loads((EXP / 'runs/late_portfolio_confirm_001/FRESH_ANALYSIS.json').read_text())
initial = json.loads((RUN / 'FRESH_ANALYSIS.json').read_text())
checks = json.loads((RUN / 'CHECKS.json').read_text())
final = json.loads((RUN / 'FINAL_AUDITS.json').read_text())
frozen = json.loads((RUN / 'frozen/FROZEN.json').read_text())
assert all(confirm['gates'].values()) and checks['generic_pair_debug_thread_all_records_equal'] and final['frozen_full_records_equal']
assert all(hashlib.sha256((ROOT / path).read_bytes()).hexdigest() == value for path, value in frozen['files_sha256'].items())


def load_games(path):
    return json.loads(path.read_text())['games']


def metrics(games):
    margins = [g['cash'] - g['opponent_cash'] for g in games]
    cash = sorted(g['cash'] for g in games)
    return {'games': len(games), 'wins': sum(m > 0 for m in margins), 'ties': sum(m == 0 for m in margins),
            'utility': statistics.mean(1 if m > 0 else .5 if m == 0 else 0 for m in margins),
            'mean_margin': statistics.mean(margins), 'mean_cash': statistics.mean(cash),
            'cvar10_margin': statistics.mean(sorted(margins)[:math.ceil(len(games) / 10)]),
            'pass_J': .8 * statistics.mean(cash) + .2 * statistics.mean(cash[:math.ceil(len(games) / 10)])}


def contrast(a, b):
    assert [(g['seed'], g['seat']) for g in a] == [(g['seed'], g['seat']) for g in b]
    ma, mb = metrics(a), metrics(b)
    result = {key + '_gain': ma[key] - mb[key] for key in ['utility', 'mean_margin', 'mean_cash', 'cvar10_margin', 'pass_J']}
    for key in ['produced', 'sold', 'discarded']:
        result[key + '_gain'] = [statistics.mean(x[key][p] - y[key][p] for x, y in zip(a, b)) for p in range(12)]
    for key in ['unit_faults', 'worker_days', 'opponent_cash']:
        result[key + '_gain'] = statistics.mean(x[key] - y[key] for x, y in zip(a, b))
    for key in ['hires', 'hire_cost']:
        result[key + '_gain'] = statistics.mean(x['profile'][key] - y['profile'][key] for x, y in zip(a, b))
    result['rival_actions_changed'] = sum(x['opponent_action_hash'] != y['opponent_action_hash'] for x, y in zip(a, b))
    return result


native = {}
for opponent in [baseline, 'late_goose_wheat_context', 'investment_context_guarded_001_best', 'teammate_shoprouter', 'king_rc4', 'public_router', 'public_router_v5', 'public_sixday', 'pass']:
    games = {a: load_games(RUN / f'native/{a}_vs_{opponent}.json') for a in [candidate, baseline]}
    native[opponent] = {'metrics': {a: metrics(g) for a, g in games.items()}, 'contrast': contrast(games[candidate], games[baseline])}
assert native[baseline]['metrics'][candidate]['utility'] > .5 and native[baseline]['metrics'][candidate]['mean_margin'] > 0
assert all(row['contrast']['mean_margin_gain'] >= 0 and row['contrast']['utility_gain'] >= 0 for row in native.values())
assert native['pass']['contrast']['pass_J_gain'] >= 0
profiles = {}
for opponent in [baseline, 'public_router']:
    games = {a: load_games(RUN / f'profiles/{a}_vs_{opponent}.json') for a in [candidate, baseline]}
    courses = {name: 0 for name in ['wait', 'goose', 'cow', 'sheep']}
    for game in games[candidate]:
        added = [life[0] for life in game['profile']['lives'] if life[1:3] == [1, 3] and life[5] == 13 and life[0] >= 9]
        assert len(added) <= 1
        courses[['wait', 'goose', 'cow', 'sheep'][added[0] - 8 if added else 0]] += 1
    profiles[opponent] = {'courses': courses, 'contrast': contrast(games[candidate], games[baseline])}
    # The context fix must preserve all tomato, berry and melon output.
    assert all(profiles[opponent]['contrast']['produced_gain'][p] == 0 for p in [2, 3, 4])
now = datetime.now(timezone.utc).isoformat()
report = {'promoted_utc': now, 'candidate': candidate, 'parent': baseline,
          'status': 'Promoted as experimental reference after independent confirmation, native, causal, operational and frozen-build checks.',
          'initial_panel': 'runs/late_portfolio_validation_001/FRESH_ANALYSIS.json',
          'initial_current_group_gain_95pct': initial['groups']['current']['gain_95pct'],
          'confirmation': confirm, 'native_and_pass': native, 'causal_profiles': profiles,
          'operational': checks, 'frozen': {'dependencies': len(frozen['files_sha256']), 'path': 'runs/late_portfolio_validation_001/frozen/FROZEN.json', 'full_native_records_exact': final['frozen_games']},
          'tail_review': 'All paired mean margins are positive. Legacy crop variants lose up to1.0742percentage points utility and about$19 worst-decile margin, within the preregistered2point per-opponent limit. Public routers improve1.37–1.44points; teammate win rate stays equal. Current and historical grouped confidence intervals are positive in the independent confirmation. Native/PASS comparisons have no utility or mean-margin regressions on the audit panel.',
          'limitations': ['One additional investment slot/date, not unrestricted cold-start composition optimization.',
                          'Sampled future shops and approximate public-herd output leave residual valuation error, especially sheep.',
                          'Daily valuation omits detailed rival crop sales and intraday liquidity; actual scheduled execution and guards remain necessary.',
                          'This is an aggregate league improvement; some older crop variants still lose a small number of wins.',
                          'No new Kaggle submission, official catalog copy or Git operation is authorized. New Thomas V5/2 port is being evaluated separately.']}
(EXP / 'results/late_portfolio_validation.json').write_text(json.dumps(report, indent=2) + '\n')
current = json.loads((EXP / 'CURRENT_REFERENCE.json').read_text())
assert current['name'] == baseline
(RUN / 'PREVIOUS_REFERENCE.json').write_text(json.dumps(current, indent=2) + '\n')
current.update(name=candidate, path='runs/late_portfolio_001/proposals/' + candidate, promoted_utc=now,
               report='results/late_portfolio_validation.json', previous=baseline)
(EXP / 'CURRENT_REFERENCE.json').write_text(json.dumps(current, indent=2) + '\n')
package = EXP / 'runs/late_portfolio_001/proposals' / candidate
lineage = json.loads((package / 'IMPORT.json').read_text())
lineage.update(status=report['status'], promoted_utc=now, validation='results/late_portfolio_validation.json')
(package / 'IMPORT.json').write_text(json.dumps(lineage, indent=2) + '\n')
readme = EXP / 'README.md'
text = readme.read_text()
text = text.replace('Current broad search starting point (18:43 UTC):\n`runs/opening_market_search_001/proposals/opening_q32_b13_v1/`.\nRead `CURRENT_REFERENCE.json` and `results/opening_market_validation.json` for\nthe promotion evidence, all tradeoffs, and the additional opening-variant league.',
    f'Current broad search starting point ({now[11:16]} UTC):\n`runs/late_portfolio_001/proposals/{candidate}/`.\nAt day13, estimate complete goose/cow/sheep calendars versus retaining crops,\nusing observed state,32 sampled shop futures and compiled labor/trade costs.\nPreserve the parent tomato intention and later observed berry branch.\nRead `CURRENT_REFERENCE.json` and `results/late_portfolio_validation.json`.\nIndependent confirmation covers180224games; direct parent1527W2256T313L,\nmean+$173.98. All paired means improve; small legacy win-rate/tail losses are\nreported. Operational, native, causal and rebuilt-source checks pass.\n\nPrevious reference (18:42 UTC):\n`runs/opening_market_search_001/proposals/opening_q32_b13_v1/`;\n`results/opening_market_validation.json` retains its evidence.')
readme.write_text(text)
entry = f'\n{now}: Promoted{candidate} over{baseline}. Independent180224-game confirmation passes all gates, current group+.1666pp(95%+.0745..+.2585), direct1527W2256T313L,+$173.98. Native/PASS no mean/utility regressions;256 rebuilt frozen records exact. Real four-way crop/animal choice, with small legacy utility/tail tradeoffs recorded. No Git or submission.\n'
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md']:
    with (EXP / name).open('a') as output:
        output.write(entry)
print('Promoted', candidate, now)
