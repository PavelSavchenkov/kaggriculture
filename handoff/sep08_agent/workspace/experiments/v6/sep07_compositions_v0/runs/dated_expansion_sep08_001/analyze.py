from datetime import datetime, timezone
from pathlib import Path
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
spec = json.loads((RUN/'LINEAGE.json').read_text())
proposals = {p['id']:p for p in json.loads((RUN/'estimated/proposals.json').read_text())}
rows = []
parity = 0
for opponent in spec['discovery']['opponents']:
    baseline = json.loads((RUN/f"discovery/{spec['baseline']}_vs_{opponent}.json").read_text())
    old = json.loads((EXP/f"runs/compiler_care_sep08_001/discovery/{spec['baseline']}_vs_{opponent}.json").read_text())
    expected = [g for g in old['games'] if g['seed']<1004]
    assert baseline['games'] == expected
    parity += len(expected)
    for name in spec['variants']:
        proposal = proposals[int(name.removeprefix('dated_expansion_p'))]
        data = json.loads((RUN/f'discovery/{name}_vs_{opponent}.json').read_text())
        games = data['games'];mean = lambda f:statistics.mean(f(g) for g in games)
        animals = {}
        for item in [9,10,11]:
            lives = [l for g in games for l in g['profile']['lives'] if l[0]==item]
            animals[str(item)] = {'placed_per_game':len(lives)/len(games),
                'birth_days':sorted(set(l[5] for l in lives)),
                'mean_birth_day':statistics.mean(l[5] for l in lives) if lives else None,
                'animal_days':sum(l[6].bit_count() for l in lives)/len(games)}
        produced = [mean(lambda g,i=i:g['produced'][i]) for i in range(9)]
        rows.append({'name':name,'id':proposal['id'],'family':proposal['family'],'day':proposal['day'],
            'count':proposal['count'],'hands':proposal['hands'],'opponent':opponent,'games':len(games),
            'utility':data['win_utility'],'cash':mean(lambda g:g['cash']),
            'margin':mean(lambda g:g['cash']-g['opponent_cash']),
            'cash_gain':mean(lambda g:g['cash'])-statistics.mean(g['cash'] for g in baseline['games']),
            'hires':mean(lambda g:g['profile']['hires']),'hire_cost':mean(lambda g:g['profile']['hire_cost']),
            'land_cost':mean(lambda g:g['profile']['land_cost']),
            'moves':mean(lambda g:sum(g['profile']['successful'][1:5])),
            'faults':mean(lambda g:g['unit_faults']),
            'produced':produced,'animals':animals,
            'estimated_cash':proposal['estimated_cash'],'estimated_margin':proposal['estimated_margin'],
            'min_estimated_cash':proposal['min_cash'],'estimated_work_gap':proposal['work_gap'],
            'estimated_produced':proposal['produced'],
            'output_difference':[a-b for a,b in zip(produced,proposal['produced'])]})


def ranks(values):
    ordered = sorted(values)
    rank = {v:statistics.mean(i for i,x in enumerate(ordered) if x==v) for v in set(values)}
    return [rank[v] for v in values]


correlations = {}
for opponent in spec['discovery']['opponents']:
    group = [r for r in rows if r['opponent']==opponent]
    correlations[opponent] = statistics.correlation(ranks([r['estimated_margin'] for r in group]),ranks([r['margin'] for r in group]))
leaders = {opponent:sorted([r for r in rows if r['opponent']==opponent],key=lambda r:(r['utility'],r['margin']),reverse=True)[:8]
           for opponent in spec['discovery']['opponents']}
report = {'created_utc':datetime.now(timezone.utc).isoformat(),'estimated':375,'selected':len(spec['variants']),
    'fullgames':sum(r['games'] for r in rows)+parity,'old_control_full_records_equal':parity,
    'rank_correlations':correlations,'leaders':leaders,'rows':rows,
    'limits':spec['limits'],
    'next':'Compare forecasts and actual birth dates/output. The highest estimates request large negative early balances; preserve this as an error and add a separate selection constrained to no extra funding deficit above the unchanged opening. Do not infer feasibility from an optimistic biological projection.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text = '# Dated expansion search results\n\n'
text += f"375 proposals,16 economic scenarios,34 selected farms; {report['fullgames']} full games including{parity} exact old controls. No strong-agent promotion.\n\n"
text += f"Estimated-versus-realized margin rank correlations: {correlations}. These are discovery results with fixed-flow estimates and live opponents.\n\n"
for opponent,group in leaders.items():
    text += f'## Against {opponent}\n\n| Agent | Family | Day/count/hands | Cash | Margin | Win utility | Estimated min cash |\n| --- | --- | --- | ---: | ---: | ---: | ---: |\n'
    for r in group:
        text += f"| {r['name']} | {r['family']} | {r['day']}/{r['count']}/{r['hands']} | {r['cash']:.2f} | {r['margin']:.2f} | {r['utility']:.3f} | {r['min_estimated_cash']:.2f} |\n"
    text += '\n'
text += report['next']+'\n'
(RUN/'RESULTS.md').write_text(text)
print('Completed',report['fullgames'],'games; rank correlations',correlations)
for opponent,group in leaders.items():
    print(opponent,[(r['name'],r['family'],round(r['cash']),round(r['margin'])) for r in group[:5]])
