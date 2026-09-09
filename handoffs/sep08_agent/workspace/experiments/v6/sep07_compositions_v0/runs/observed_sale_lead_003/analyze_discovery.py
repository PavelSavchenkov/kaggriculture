"""Audit sale timing, implementation parity and paired league discovery."""
from datetime import datetime, timezone
from pathlib import Path
from statistics import mean
import hashlib
import json
import math

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
BASE = 'rival_wool_context_v3'
NAMES = ['sale_lead_shared_control', 'observed_sale_lead_shared_v3', 'observed_sale_lead_shared_milk_wool']
OPPONENTS = ['rival_wool_context_v3', 'ahmed_v23', 'public_router_v52', 'public_router_v5', 'teammate_shoprouter', 'public_router', 'junghoon_wool_sales', 'john_131', 'king_rc4', 'investment_context_guarded_001_best']
EARLIER = dict(zip(NAMES, ['sale_lead_fast_control', 'observed_sale_lead_fast_v2', 'observed_sale_lead_fast_milk_wool']))
expected = [(s, seat) for s in range(1000, 1128) for seat in range(2)]
rows, hashes = {}, {}


def read(path):
    data = json.loads(path.read_text())
    assert [(g['seed'], g['seat']) for g in data['games']] == expected
    assert all(g['turns'] == 719 for g in data['games'])
    hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
    return data


for opponent in OPPONENTS:
    baseline = read(RUN / f'discovery/{BASE}_vs_{opponent}.json')
    rows[opponent] = {}
    for agent in NAMES:
        data = read(RUN / f'discovery/{agent}_vs_{opponent}.json')
        games, controls = data['games'], baseline['games']
        delta = [(a['cash'] - a['opponent_cash']) - (b['cash'] - b['opponent_cash']) for a, b in zip(games, controls)]
        margins = [g['cash'] - g['opponent_cash'] for g in games]
        row = {
            'games': len(games), 'wins': sum(x > 0 for x in margins), 'ties': sum(x == 0 for x in margins),
            'utility': data['win_utility'], 'utility_gain': data['win_utility'] - baseline['win_utility'],
            'mean_margin': data['mean_margin'], 'mean_margin_gain': mean(delta),
            'margin_cvar10_gain': data['margin_cvar10'] - baseline['margin_cvar10'],
            'paired_margin_min': min(delta), 'paired_margin_better': sum(x > 0 for x in delta),
            'paired_margin_worse': sum(x < 0 for x in delta),
            'mean_own_cash_gain': mean(a['cash'] - b['cash'] for a, b in zip(games, controls)),
            'mean_rival_cash_gain': mean(a['opponent_cash'] - b['opponent_cash'] for a, b in zip(games, controls)),
            'mean_unit_faults_gain': mean(a['unit_faults'] - b['unit_faults'] for a, b in zip(games, controls)),
            'mean_worker_days_gain': mean(a['worker_days'] - b['worker_days'] for a, b in zip(games, controls)),
            'mean_production_gain': [mean(a['produced'][i] - b['produced'][i] for a, b in zip(games, controls)) for i in range(len(games[0]['produced']))],
            'production_equal_games': sum(a['produced'] == b['produced'] for a, b in zip(games, controls)),
            'rival_actions_changed': sum(a['opponent_action_hash'] != b['opponent_action_hash'] for a, b in zip(games, controls)),
            'seconds': data['seconds'], 'baseline_seconds': baseline['seconds'],
            'full_records_equal': games == controls,
        }
        older = EXP / f'runs/observed_sale_lead_002/discovery/{EARLIER[agent]}_vs_{opponent}.json'
        if older.exists():
            old = read(older)
            row['v2_full_records_equal'] = games == old['games']
            row['v2_seconds'] = old['seconds']
            assert row['v2_full_records_equal']
        if agent == NAMES[0]:
            assert row['full_records_equal'], 'Prediction-only control must preserve all original records.'
        rows[opponent][agent] = row
        print(opponent, agent, 'utility_gain', round(row['utility_gain'], 5), 'margin_gain', round(row['mean_margin_gain'], 2), 'paired_worse', row['paired_margin_worse'], flush=True)

summary = {a: {key: mean(rows[o][a][key] for o in OPPONENTS) for key in ['utility_gain', 'mean_margin_gain', 'mean_own_cash_gain', 'mean_rival_cash_gain', 'seconds']} for a in NAMES}
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'scope': 'Exposed discovery1000..1127, both seats, ten opponents; no fresh promotion claim.', 'games': 4 * len(expected) * len(OPPONENTS), 'comparisons': rows, 'equal_opponent_means': summary, 'source_sha256': hashes}
(RUN / 'DISCOVERY_ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
print('Equal-opponent means', summary)
