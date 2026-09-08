from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
variants = json.loads((RUN / 'LINEAGE.json').read_text())['variants']
opponents = ['observed_sale_lead_start_216', 'public_router_v52', 'ahmed_v23', 'junghoon_wool_sales', 'john_131', 'teammate_shoprouter', 'king_rc4']
rows = []
controls = 0
for opponent in opponents:
    data = {name: json.loads((RUN / f'discovery/{name}_vs_{opponent}.json').read_text()) for name in variants + ['public_capacity_router', 'public_terminal_router']}
    for name, original in [('arlene_v4_m0', 'public_capacity_router'), ('arlene_v4_m4', 'public_terminal_router')]:
        assert data[name]['games'] == data[original]['games']
        controls += len(data[name]['games'])
    base = data['arlene_v4_m0']
    for name in variants:
        d = data[name]
        mean = lambda f: statistics.mean(f(g) for g in d['games'])
        rows.append({'name': name, 'opponent': opponent, 'games': len(d['games']), 'utility': d['win_utility'],
            'cash': mean(lambda g: g['cash']), 'margin': mean(lambda g: g['cash'] - g['opponent_cash']),
            'margin_gain': mean(lambda g: g['cash'] - g['opponent_cash']) - statistics.mean(g['cash'] - g['opponent_cash'] for g in base['games']),
            'unchanged_full_records': sum(a == b for a, b in zip(d['games'], base['games'])),
            'produced': [mean(lambda g, i=i: g['produced'][i]) for i in range(9)],
            'hires': mean(lambda g: g['profile']['hires']), 'hire_cost': mean(lambda g: g['profile']['hire_cost']),
            'moves': mean(lambda g: sum(g['profile']['successful'][1:5])), 'faults': mean(lambda g: g['unit_faults'])})
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': 8960,
    'original_control_full_records_equal': controls, 'rows': rows,
    'source_parity': 'SOURCE_PARITY.json', 'operational': 'OPERATIONAL_CHECKS.json',
    'decision': 'No strongest-agent promotion or new official agent. Full public port wins15/128 against incumbent; separate clamp causes about25k mean-margin losses against V5/2 and Ahmed, without changing aggregate win counts on this panel. Inspect exact market/finance trace before borrowing that clamp.',
    'limits': 'Exposed discovery seeds, seven opponents,128games each. No fresh promotion audit or global-rank inference. Budget activates on source fixtures but its isolated full-game records equal the base in this panel.'}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
hashes = {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [RUN / 'source/agent.cpp', RUN / 'source/agent.hpp', RUN / 'budget.hpp']}
(RUN / 'SOURCE_HASHES.json').write_text(json.dumps(hashes, indent=2) + '\n')
text = '# Arlene V4 public component comparison\n\n' + report['decision'] + '\n\n'
text += f'8,628 source actions match across all four routes. 8,960 full profiled discovery games; {controls} complete existing control records exact. 80 operational games pass.\n\n'
text += '| Opponent | Base score | Full score | Full mean margin | Change |\n| --- | ---: | ---: | ---: | ---: |\n'
for opponent in opponents:
    base = next(r for r in rows if r['opponent'] == opponent and r['name'] == 'arlene_v4_m0')
    full = next(r for r in rows if r['opponent'] == opponent and r['name'] == 'arlene_v4_m31')
    text += f"| {opponent} | {base['utility']:.3%} | {full['utility']:.3%} | {full['margin']:.2f} | {full['margin_gain']:+.2f} |\n"
text += '\nBudget-only results equal the original complete records in all seven opponent panels. Final settlement and padded-worker repair also have no measured benefit here. Source parity and ablations prevent treating public notebook claims as strength evidence.\n'
(RUN / 'RESULTS.md').write_text(text)
(RUN / 'README.md').write_text('# Arlene V4 source port and ablations\n\nRead RESULTS.md and LINEAGE.json. prepare.py reconstructs the typed source and named local policies; verify_source.py compares8,628 public-source actions; run_discovery.py runs the seven-opponent panel; check_operations.py is independent of the discovery build. Full mask31, components bits1/2/4/8/16. Failed first source and orchestration details are preserved in PARITY_AMENDMENT.json and review77. No promotion.\n')
print('8960 games;', controls, 'exact existing control records.')
for row in rows:
    if row['name'] == 'arlene_v4_m31':
        print(row['opponent'], row['utility'], round(row['margin_gain'], 3))
