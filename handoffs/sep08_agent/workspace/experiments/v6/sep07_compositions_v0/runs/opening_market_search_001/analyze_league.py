from pathlib import Path
import csv
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
names = [row['name'] for row in csv.DictReader((RUN / 'discovery/choices.csv').open())]
measurements = {}
hashes = {}
for path in sorted((RUN / 'league_discovery').glob('*.json')):
    data = json.loads(path.read_text())
    a, b = data['agent_a'], data['agent_b']
    games = data['games']
    assert len(games) == 128 and all(g['turns'] == 719 for g in games)
    assert (a, b) not in measurements
    margin = [g['cash'] - g['opponent_cash'] for g in games]
    wins, ties = sum(m > 0 for m in margin), sum(m == 0 for m in margin)
    value = {'games': 128, 'wins': wins, 'ties': ties, 'utility': (wins + ties / 2) / 128,
             'mean_margin': statistics.mean(margin)}
    measurements[a, b] = value
    if a != b:
        measurements[b, a] = {**value, 'wins': 128 - wins - ties,
                              'utility': 1 - value['utility'], 'mean_margin': -value['mean_margin']}
    else:
        assert value['utility'] == 0.5 and value['mean_margin'] == 0
    hashes[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
assert len(hashes) == 190 and len(measurements) == len(names) ** 2
rows = []
for name in names:
    opponents = {b: measurements[name, b] for b in names if b != name}
    rows.append({'name': name, 'mean_utility': statistics.mean(v['utility'] for v in opponents.values()),
                 'worst_utility': min(v['utility'] for v in opponents.values()),
                 'mean_margin': statistics.mean(v['mean_margin'] for v in opponents.values()),
                 'winning_matchups': sum(v['utility'] > 0.5 for v in opponents.values()),
                 'opponents': opponents})
rows.sort(key=lambda row: (row['mean_utility'], row['mean_margin']), reverse=True)
report = {'scope': 'Discovery, 64 common seeds in both seats; 19 choices, 190 canonical cohorts including self, 24320 full games. Reverse directions use paired seat symmetry. No fresh selection claim.',
          'ranking': rows, 'source_sha256': hashes}
(RUN / 'LEAGUE_SCREEN.json').write_text(json.dumps(report, indent=2) + '\n')
for row in rows:
    print(row['name'], 'utility', round(row['mean_utility'], 6), 'worst', row['worst_utility'],
          'winning_matchups', row['winning_matchups'], 'margin', round(row['mean_margin'], 2))
for opponent in names:
    print('q32_b13_m2 vs', opponent, measurements['q32_b13_m2', opponent])
