from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
result = {}
for folder, count in [('league_fresh', 1024), ('league_native', 256)]:
    rows = {}
    for path in sorted((RUN / folder).glob('*.json')):
        data = json.loads(path.read_text())
        games = data['games']
        assert len(games) == count and all(g['turns'] == 719 for g in games)
        margin = [g['cash'] - g['opponent_cash'] for g in games]
        wins, ties = sum(m > 0 for m in margin), sum(m == 0 for m in margin)
        rows[data['agent_b']] = {'games': count, 'wins': wins, 'ties': ties, 'losses': count - wins - ties,
            'utility': (wins + ties / 2) / count, 'mean_margin': statistics.mean(margin),
            'margin_cvar10': statistics.mean(sorted(margin)[:(count + 9) // 10]),
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
    assert len(rows) == 19
    assert all(row['utility'] >= .48 for row in rows.values())
    for name in ['q81_b13_m0', 'q81_b13_m2']:
        assert rows[name]['utility'] > .5 and rows[name]['mean_margin'] > 0
    result[folder] = rows
report = {'created_utc': datetime.now(timezone.utc).isoformat(), 'status': 'All preregistered additional league gates passed',
          'candidate': 'q32_b13_m2', 'panels': result,
          'correction': 'Earlier control counterexample compared cash earned against different opponents. Direct discovery and fresh games instead favor q32.',
          'games': sum(row['games'] for rows in result.values() for row in rows.values())}
(RUN / 'FRESH_LEAGUE_VALIDATION.json').write_text(json.dumps(report, indent=2) + '\n')
for label, rows in result.items():
    print(label, 'worst', min((r['utility'], n) for n, r in rows.items()))
    for name in ['q81_b13_m0', 'q81_b13_m2', 'q0_b13_m2']:
        print(name, rows[name])
