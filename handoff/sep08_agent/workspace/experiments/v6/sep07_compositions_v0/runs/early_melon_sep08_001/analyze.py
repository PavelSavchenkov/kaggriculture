from pathlib import Path
import json,statistics

RUN=Path(__file__).resolve().parent
names=json.loads((RUN/'LINEAGE.json').read_text())['variants']
opponents=['empty_sale_slots_m2','public_router','service_bank_p362_m2','cold_renewal_p98','pass']
rows=[];control_games=0
for opponent in opponents:
    get=lambda n:json.loads((RUN/'discovery'/f'{n}_vs_{opponent}.json').read_text())['games']
    for base in [0,50,98,157]:
        baseline=get(f'early_melon_b{base}_m0')
        assert baseline==get(f'cold_renewal_p{base}');control_games+=len(baseline)
        for mode in range(2 if base==0 else 4):
            name=f'early_melon_b{base}_m{mode}';games=get(name)
            assert [(g['seed'],g['seat']) for g in games]==[(g['seed'],g['seat']) for g in baseline]
            delta=lambda fn:statistics.mean(fn(a)-fn(b) for a,b in zip(games,baseline))
            melons=[l for g in games for l in g['profile']['lives'] if l[0]==4]
            before=[l for g in baseline for l in g['profile']['lives'] if l[0]==4]
            rows.append({'agent':name,'base':base,'mode':mode,'opponent':opponent,'games':len(games),
                'wtl':[sum((g['cash']>g['opponent_cash'],g['cash']==g['opponent_cash'],g['cash']<g['opponent_cash'])[i] for g in games) for i in range(3)],
                'cash':statistics.mean(g['cash'] for g in games),'margin':statistics.mean(g['cash']-g['opponent_cash'] for g in games),
                'cash_gain':delta(lambda g:g['cash']),'margin_gain':delta(lambda g:g['cash']-g['opponent_cash']),
                'hire_cost_gain':delta(lambda g:g['profile']['hire_cost']),'fault_gain':delta(lambda g:g['unit_faults']),
                'produced':[statistics.mean(g['produced'][i] for g in games) for i in range(9)],
                'production_gain':[delta(lambda g:g['produced'][i]) for i in range(9)],
                'melon_output_per_game_range':[min(g['produced'][4] for g in games),max(g['produced'][4] for g in games)],
                'mean_melon_end_day':statistics.mean((l[4]-1)//24 for l in melons),
                'mean_melon_end_step_gain':statistics.mean(l[4] for l in melons)-statistics.mean(l[4] for l in before)})
assert control_games==320
summary=[]
for name in names:
    strong=[r for r in rows if r['agent']==name and r['opponent'] in opponents[:2]]
    summary.append({'agent':name,'strong_cash_gain':statistics.mean(r['cash_gain'] for r in strong),
        'strong_margin_gain':statistics.mean(r['margin_gain'] for r in strong),
        'strong_melon_output':statistics.mean(r['produced'][4] for r in strong),
        'mean_melon_end_step_gain':statistics.mean(r['mean_melon_end_step_gain'] for r in strong)})
report={'discovery_games':1440,'exact_control_games':control_games,'rows':rows,'summary':summary,
    'scope':'Exposed discovery seeds1000..1007, both seats. Each gain uses its own unchanged farm-family control. Full-game melon output and end dates distinguish intended from realized earlier harvest.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Earlier melon harvest and crop renewal\n\n1440 full games;320 exact old-family controls. Gains below use the same family, two strong opponents, and common seeds.\n\n| Variant | Cash gain | Margin gain | Melon output | Mean melon end-step change |\n| --- | ---: | ---: | ---: | ---: |\n'
for r in summary:text+=f"| {r['agent']} | {r['strong_cash_gain']:+.2f} | {r['strong_margin_gain']:+.2f} | {r['strong_melon_output']:.2f} | {r['mean_melon_end_step_gain']:+.2f} |\n"
text+='\nMode1 advances harvest only; mode2 also regenerates renewal dates; mode3 permits same-day renewal. See ANALYSIS.json for direct cold-parent results, all products, faults, labor and actual melon yield ranges. These are not fresh promotion results.\n'
(RUN/'RESULTS.md').write_text(text);print(text)
