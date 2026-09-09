"""Evaluate the preregistered broad audit; do not change the accepted agent."""
from pathlib import Path
import hashlib
import json
import numpy as np

RUN = Path(__file__).resolve().parent
OUT = RUN / 'broad_2360000'
ROOT = RUN.parents[4]
protocol = json.loads((OUT / 'PROTOCOL.json').read_text())
execution = json.loads((OUT / 'EXECUTION.json').read_text())
assert execution['sources_unchanged'] and execution['games'] == protocol['games']
frozen = json.loads((OUT / 'freeze/FROZEN.json').read_text())['files_sha256']
assert all(hashlib.sha256((ROOT / p).read_bytes()).hexdigest() == h for p, h in frozen.items())
assert json.loads((OUT / 'TELEMETRY_PARITY.json').read_text())['full_records_equal'] == 12
agents = protocol['agents']
baseline = 'empty_sale_slots_m2'
candidates = [a for a in agents if a != baseline]
pairs = [(a, baseline) for a in candidates] + [(candidates[1], candidates[0])]
rows, missed, counts = [], [], []
series, metrics = {}, {}

def score(g):
    return float(g['cash'] > g['opponent_cash']) + .5 * (g['cash'] == g['opponent_cash'])

def features(g):
    return [score(g), g['cash'], g['cash'] - g['opponent_cash']]

def metric(games):
    data = np.array([features(g) for g in games])
    wins = int((data[:, 0] == 1).sum())
    ties = int((data[:, 0] == .5).sum())
    tail = max(1, (len(data) + 9) // 10)
    return {'games': len(data), 'wtl': [wins, ties, len(data) - wins - ties],
        'utility': float(data[:, 0].mean()), 'cash': float(data[:, 1].mean()), 'margin': float(data[:, 2].mean()),
        'cash_cvar10': float(np.sort(data[:, 1])[:tail].mean()), 'margin_cvar10': float(np.sort(data[:, 2])[:tail].mean())}

for native in (False, True):
    panel = 'native' if native else 'fresh'
    opponents = protocol['native_opponents'] if native else protocol['opponents']
    prefix = 'native_' if native else ''
    for b in opponents:
        games = {a: json.loads((OUT / f'{prefix}{a}_vs_{b}.json').read_text())['games'] for a in agents}
        for a, records in games.items():
            assert all(g['turns'] == 719 for g in records)
            metrics[panel, a, b] = metric(records)
            diagnostics = json.loads((OUT / f'{prefix}{a}_vs_{b}.json.diagnostics.json').read_text())['games']
            assert [(g['seed'], g['seat']) for g in records] == [(g['seed'], g['seat']) for g in diagnostics]
            counts.append({'panel': panel, 'candidate': a, 'opponent': b,
                'active': sum(d['family'] >= 0 for d in diagnostics),
                'repaired': sum(bool(d['repaired_days']) for d in diagnostics),
                'missed': sum(bool(d['missed_days']) for d in diagnostics)})
            for d, g in zip(diagnostics, records):
                if d['missed_days']:
                    missed.append(dict(d, panel=panel, candidate=a, opponent=b,
                        cash=g['cash'], opponent_cash=g['opponent_cash'], unit_faults=g['unit_faults']))
        for a, parent in pairs:
            x, y = games[a], games[parent]
            assert [(g['seed'], g['seat']) for g in x] == [(g['seed'], g['seat']) for g in y]
            delta = np.array([features(g) for g in x]) - np.array([features(g) for g in y])
            series[panel, a, parent, b] = delta.reshape(-1, 2, 3).mean(axis=1)
            rows.append({'panel': panel, 'candidate': a, 'parent': parent, 'opponent': b,
                'candidate_metrics': metrics[panel, a, b], 'parent_metrics': metrics[panel, parent, b],
                'gain_utility_pp': float(delta[:, 0].mean() * 100),
                'gain_cash': float(delta[:, 1].mean()), 'gain_margin': float(delta[:, 2].mean()),
                'gain_faults': float(np.mean([g['unit_faults'] - c['unit_faults'] for g, c in zip(x, y)])),
                'gain_hire_cost': float(np.mean([g['profile']['hire_cost'] - c['profile']['hire_cost'] for g, c in zip(x, y)])),
                'produced_gain': np.mean([np.array(g['produced']) - c['produced'] for g, c in zip(x, y)], axis=0).tolist()})

groups = []
for panel in ('fresh', 'native'):
    labels = ('primary', 'historical') if panel == 'fresh' else ('primary',)
    for a, parent in pairs:
        for label in labels:
            members = protocol[label]
            values = np.stack([series[panel, a, parent, b] for b in members]).mean(axis=0)
            rng = np.random.default_rng(2360000)
            sampled = values[rng.integers(0, len(values), size=(4000, len(values)))].mean(axis=1)
            bounds = np.quantile(sampled, [.025, .975], axis=0)
            groups.append({'panel': panel, 'candidate': a, 'parent': parent, 'group': label,
                'opponents': members, 'candidate_utility': float(np.mean([metrics[panel, a, b]['utility'] for b in members])),
                'parent_utility': float(np.mean([metrics[panel, parent, b]['utility'] for b in members])),
                'gain_utility_pp': float(values[:, 0].mean() * 100), 'gain_cash': float(values[:, 1].mean()),
                'gain_margin': float(values[:, 2].mean()), 'gain_95pct': {'utility_pp': (bounds[:, 0] * 100).tolist(),
                    'cash': bounds[:, 1].tolist(), 'margin': bounds[:, 2].tolist()}})

eligibility = []
for a in candidates:
    own_groups = {(g['panel'], g['group']): g for g in groups if g['candidate'] == a and g['parent'] == baseline}
    gates = {'operational_and_source_checks': True}
    for key, g in own_groups.items():
        historical = key[1] == 'historical'
        for metric_name in ('utility_pp', 'margin'):
            low = g['gain_95pct'][metric_name][0]
            gates['_'.join(key) + '_' + metric_name] = low >= -1e-12 if historical else low > 0
    gates['wins_every_primary_matchup'] = all(metrics[panel, a, b]['utility'] > .5 for panel in ('fresh', 'native') for b in protocol['primary'])
    gates['no_uninvestigated_guard_misses'] = not any(d['candidate'] == a for d in missed)
    eligibility.append({'candidate': a, 'gates': gates, 'eligible_before_frozen_rebuild': all(gates.values()),
        'primary_utility': own_groups['fresh', 'primary']['candidate_utility'],
        'primary_margin_gain': own_groups['fresh', 'primary']['gain_margin']})
eligible = [e for e in eligibility if e['eligible_before_frozen_rebuild']]
selected = max(eligible, key=lambda e: (e['primary_utility'], e['primary_margin_gain']))['candidate'] if eligible else None
report = {'games': protocol['games'], 'rows': rows, 'groups': groups, 'activation_counts': counts,
    'missed_guards': missed, 'eligibility': eligibility, 'selected_before_final_audits': selected,
    'scope': 'Preregistered independent population comparison. No automatic promotion. Any missed guard requires a separate trace; frozen-source rebuild and per-opponent tail review remain final obligations.'}
(RUN / 'BROAD_RESULTS.json').write_text(json.dumps(report, indent=2) + '\n')
body = f"# Broad independent comparison\n\n{protocol['games']:,} games; frozen sources unchanged. No promotion is performed.\n\n"
body += '| Panel/group | Candidate | Parent | Win-score gain, pp | 95% interval | Mean margin gain |\n| --- | --- | --- | ---: | --- | ---: |\n'
for g in groups:
    low, high = g['gain_95pct']['utility_pp']
    body += f"| {g['panel']}/{g['group']} | {g['candidate']} | {g['parent']} | {g['gain_utility_pp']:+.3f} | [{low:+.3f}, {high:+.3f}] | {g['gain_margin']:+.2f} |\n"
body += f'\nSelected before final audits: {selected}. Missed-guard games: {len(missed)}.\n'
body += '\nDetailed per-opponent wins, cash, margins, worst-decile results, production, hires and guard records are in BROAD_RESULTS.json.\n'
(RUN / 'BROAD_RESULTS.md').write_text(body)
print(json.dumps({'eligibility': eligibility, 'selected': selected, 'missed_guards': len(missed)}, indent=2))
