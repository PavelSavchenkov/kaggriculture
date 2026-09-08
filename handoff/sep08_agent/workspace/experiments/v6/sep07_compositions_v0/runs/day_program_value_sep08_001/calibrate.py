from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import numpy as np

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
analysis = json.loads((RUN / 'ANALYSIS.json').read_text())
coverage = json.loads((RUN / 'COVERAGE.json').read_text())
operations = json.loads((RUN / 'OPERATIONAL_CHECKS.json').read_text())
assert analysis['exact_old_controls'] == 128 and coverage['full_game_records_equal'] == 512 and operations['games'] == 104
for p, digest in json.loads((RUN / 'LINEAGE.json').read_text())['source_hashes'].items():
    assert hashlib.sha256((EXP / p).read_bytes()).hexdigest() == digest, p
rows = []
for case in ['p355', 'p362']:
    for opponent in ['public_router', 'observed_sale_lead_start_216', f'joint_routes_{case}_m0', 'pass']:
        original = json.loads((EXP / f'runs/day_programs_sep08_001/discovery/day_program_{case}_m0_vs_{opponent}.json').read_text())['games']
        for mode in [1, 2, 3]:
            name = f'day_value_{case}_m{mode}_vs_{opponent}'
            games = json.loads((RUN / f'discovery/{name}.json').read_text())['games']
            measurements = [json.loads(line) for line in (RUN / f'coverage_results/{name}.coverage.jsonl').read_text().splitlines()]
            for a, b, c in zip(games, original, measurements):
                assert (a['seed'],a['seat']) == (b['seed'],b['seat']) == (c['seed'],c['seat'])
                rows.append({'case': case, 'opponent': opponent, 'mode': mode, 'seed': a['seed'], 'seat': a['seat'],
                    'predicted_gain_sum': c['predicted_gain'], 'forecast_steps': c['forecast_steps'],
                    'actual_cash_gain_vs_original': a['cash']-b['cash'],
                    'actual_margin_gain_vs_original': a['cash']-a['opponent_cash']-b['cash']+b['opponent_cash'],
                    'active_days': int(c['days']).bit_count(), 'max_act_ms': c['max_act_ms'], 'total_act_ms': c['total_act_ms']})
summary = {}
for mode in [1, 2, 3]:
    data = [r for r in rows if r['mode'] == mode]
    active = [r for r in data if r['active_days']]
    pred = np.array([r['predicted_gain_sum'] for r in active])
    actual = np.array([r['actual_cash_gain_vs_original'] for r in active])
    summary[mode] = {'games': len(data), 'activated_games': len(active),
        'mean_predicted_gain_sum': float(np.mean([r['predicted_gain_sum'] for r in data])),
        'mean_actual_cash_gain_vs_original': float(np.mean([r['actual_cash_gain_vs_original'] for r in data])),
        'mean_actual_margin_gain_vs_original': float(np.mean([r['actual_margin_gain_vs_original'] for r in data])),
        'positive_predicted_negative_realized_cash_games': int(((pred > 0) & (actual < 0)).sum()),
        'prediction_cash_gain_correlation_active_games': float(np.corrcoef(pred, actual)[0,1]) if pred.std() and actual.std() else None,
        'mean_forecast_steps': float(np.mean([r['forecast_steps'] for r in data])),
        'max_act_ms_under_concurrent_load': max(r['max_act_ms'] for r in data),
        'mean_total_act_ms_under_concurrent_load': float(np.mean([r['total_act_ms'] for r in data]))}
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'rows': rows, 'summary': summary,
    'scope': 'Episode-level calibration only: predicted values sum conditional decisions made at different reached states, so their sum is not an unbiased single-policy forecast. Actual outcomes compare complete policies with the original compiler on matched seeds/seats.',
    'decision': 'No horizon broadly improves the relaxed day-program controller. One-day decisions reproduce it exactly on all128 controls; longer forecasts change decisions with mixed gains and losses. Keep the reusable physical validator, but do not replace the compiler or best agent with these value selectors.',
    'next': 'Improve day contracts and scenario models before increasing horizon or candidate count. Diagnose missing current-state service tasks and use observed rival supply and uncertain future shops when comparing longer continuations.'}
(RUN / 'CALIBRATION.json').write_text(json.dumps(report, indent=2)+'\n')
text = '# Continuation forecast calibration\n\n'+report['decision']+'\n\n'+report['scope']+'\n\n| Horizon | Active /128 | Mean predicted sum | Mean actual cash gain vs original | Positive forecast, negative actual | Mean simulated steps | Maximum act ms* |\n|---|---:|---:|---:|---:|---:|---:|\n'
for mode, v in summary.items():
    text += f"| {['1 day','3 days','remaining season'][mode-1]} | {v['activated_games']} | {v['mean_predicted_gain_sum']:.2f} | {v['mean_actual_cash_gain_vs_original']:.2f} | {v['positive_predicted_negative_realized_cash_games']} | {v['mean_forecast_steps']:.1f} | {v['max_act_ms_under_concurrent_load']:.2f} |\n"
text += '\n*Measured under concurrent audit load; not a controlled throughput comparison.\n\n'+report['next']+'\n'
(RUN / 'CALIBRATION.md').write_text(text)
paths = [RUN / 'policy.hpp', *sorted((RUN / 'proposals').rglob('*.hpp')), *sorted((RUN / 'proposals').rglob('*.cpp'))]
(RUN / 'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2)+'\n')
print(report['decision'])
for mode, values in summary.items(): print(mode, values)
