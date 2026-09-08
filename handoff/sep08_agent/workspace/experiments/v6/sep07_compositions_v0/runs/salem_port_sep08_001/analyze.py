"""Record paired component effects and retain the useful public farm calendar."""
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from statistics import mean
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
discovery = RUN / 'discovery'
execution = json.loads((discovery / 'EXECUTION.json').read_text())
assert execution['sources_unchanged'] and execution['games'] == 3968
opponents = ['empty_sale_slots_m2', 'teammate_shoprouter', 'public_router', 'public_router_v52', 'ahmed_v23', 'junghoon_wool_sales', 'king_rc4']
data = {(a, b): json.loads((discovery / f'{a}_vs_{b}.json').read_text())
    for a in [f'salem_sep08_m{i}' for i in range(4)] for b in opponents}
def wtl(games):
    return [sum(g['cash'] > g['opponent_cash'] for g in games),
            sum(g['cash'] == g['opponent_cash'] for g in games),
            sum(g['cash'] < g['opponent_cash'] for g in games)]
def metrics(games):
    return {'wtl': wtl(games), 'games': len(games), 'cash': mean(g['cash'] for g in games),
        'margin': mean(g['cash']-g['opponent_cash'] for g in games),
        'hires': mean(g['profile']['hires'] for g in games),
        'hire_cost': mean(g['profile']['hire_cost'] for g in games),
        'faults': mean(g['unit_faults'] for g in games),
        'production': [mean(g['produced'][i] for g in games) for i in range(12)]}
rows = []
for opponent in opponents:
    row = {'opponent': opponent}
    for mode in range(4):
        games = data[(f'salem_sep08_m{mode}', opponent)]['games']
        assert len(games) == 128 and all(g['turns'] == 719 for g in games)
        row[f'm{mode}'] = metrics(games)
    rows.append(row)
effects = {}
for label, baseline in [('weed_on_top_of_sales', 2), ('sales_on_top_of_weed', 1), ('combined_vs_tape', 0)]:
    changes = []
    for opponent in opponents:
        left = data[('salem_sep08_m3', opponent)]['games']
        right = data[(f'salem_sep08_m{baseline}', opponent)]['games']
        assert [(g['seed'], g['seat'], g['shops']) for g in left] == [(g['seed'], g['seat'], g['shops']) for g in right]
        for a, b in zip(left, right):
            changes.append({'cash': a['cash']-b['cash'],
                'margin': (a['cash']-a['opponent_cash'])-(b['cash']-b['opponent_cash']),
                'faults': a['unit_faults']-b['unit_faults'],
                'hire_cost': a['profile']['hire_cost']-b['profile']['hire_cost'],
                'utility': (int(a['cash']>a['opponent_cash'])+.5*int(a['cash']==a['opponent_cash']))
                    -(int(b['cash']>b['opponent_cash'])+.5*int(b['cash']==b['opponent_cash']))})
    effects[label] = {k: mean(x[k] for x in changes) for k in changes[0]}
direct = {f'm{i}': metrics(json.loads((discovery / f'salem_sep08_m3_vs_salem_sep08_m{i}.json').read_text())['games']) for i in range(3)}
sample = data[('salem_sep08_m3', 'empty_sale_slots_m2')]['games'][0]
items = 'WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP'.split()
calendar = Counter((items[x[0]], x[5], min(30,(x[4]+23)//24)) for x in sample['profile']['lives'])
(RUN / 'OBSERVED_CALENDAR.json').write_text(json.dumps({'source': 'Full C++ port, discovery vs empty_sale_slots_m2, seed1000 seat0.',
    'scope': 'Observed dates and aggregate counts; not an optimal service prescription or original replay attribution.',
    'lifetimes': [{'item': k[0], 'start_day': k[1], 'end_day_ceiling': k[2], 'count': n} for k, n in sorted(calendar.items())],
    'raw_lives': sample['profile']['lives']}, indent=2) + '\n')
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': 3968, 'opponents': rows,
    'paired_effects': effects, 'direct_components': direct,
    'decision': 'Do not promote. Full port loses every discovery game against six of seven strong opponents and splits against Junghoon. Keep as a diagnostic public opponent and source of crop calendars/weed repair.',
    'limits': 'Exposed discovery seeds only. No official score or donor-local-result reproduction. No confidence claim from direct counters to tape siblings.'}
(RUN / 'RESULTS.json').write_text(json.dumps(report, indent=2) + '\n')
body = '''# Salem public controller: local results

The full C++ port passes 11504 source actions and 48 operational games.
The 3968-game discovery freezes271 actual C++ dependencies and verifies them
unchanged after execution. All matches have719 transitions and use common
64 seeds/both seats. This is a weaker public opponent, not a promotion.

| Opponent | Full port W/T/L | Mean cash margin |
| --- | ---: | ---: |
'''
for row in rows:
    value = row['m3']
    body += f"| {row['opponent']} | {'/'.join(map(str,value['wtl']))} | {value['margin']:+.2f} |\n"
body += '\nPaired component effects across the seven equally weighted opponents:\n\n| Change | Own cash | Match margin | Utility pp | Failed unit actions |\n| --- | ---: | ---: | ---: | ---: |\n'
for label, value in effects.items():
    body += f"| {label} | {value['cash']:+.2f} | {value['margin']:+.2f} | {value['utility']*100:+.3f} | {value['faults']:+.2f} |\n"
body += '''
The source's fixed 8-cow/4-sheep farm includes substantially different wheat,
melon and strawberry calendars. OBSERVED_CALENDAR.json preserves one executed
full farm for composition proposals. The weed repair is the larger broad gain;
earlier sales are smaller against this panel, despite a large direct advantage
against the weed-only sibling. A direct sibling win is not a broad strength gain.

No policy is promoted. Do not spend a fresh promotion panel on this controller.
Retain its exact attribution and negative results; reuse useful dated crop
courses or tested repair behavior only with local causal ablations.
'''
(RUN / 'RESULTS.md').write_text(body)
lineage = json.loads((RUN / 'LINEAGE.json').read_text())
lineage['validation'] = 'SOURCE_PARITY.json:11504 actions. OPERATIONAL_CHECKS.json:48 games. RESULTS.json:3968 discovery games; no promotion.'
(RUN / 'LINEAGE.json').write_text(json.dumps(lineage, indent=2) + '\n')
for p in (RUN / 'proposals').glob('*/README.md'):
    text = p.read_text().replace('Source parity, operational checks and local playing strength remain unverified.',
        'Full-mode source parity11504 actions and all-mode operational48 games pass.\nThe3968-game component screen finds the full port weaker than six strong\nopponents; no promotion. See ../../RESULTS.md for exact effects and limits.')
    p.write_text(text)
stamp = datetime.now(timezone.utc).isoformat()
line = f'\n{stamp}: Salem C++ port complete:11504 action parity/48 operations/3968 frozen discovery games. Full mode0/128 vs each of6 strong opponents,64/128 vsJunghoon. Keep dated crop calendar and weed repair as ideas; no promotion. Paired effects {json.dumps(effects)}. See runs/salem_port_sep08_001/RESULTS.md.\n'
for name in ['PROGRESS.md','IDEAS_LEDGER.md','PROFILING_LEDGER.md']:
    with (EXP / name).open('a') as out:
        out.write(line)
print(json.dumps({'paired_effects': effects, 'direct_components': {k:{j:v[j] for j in ['wtl','margin']} for k,v in direct.items()}}, indent=2))
