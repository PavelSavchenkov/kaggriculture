from pathlib import Path
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
model = json.loads((RUN/'MODEL.json').read_text())
keys = json.loads((RUN/'KEYS.json').read_text())
assert json.loads((RUN/'REVERSE_EXPORT.json').read_text())['all_cash_action_hashes_profiles_exact']
lookup = {(k['actor'],k['opponent'],k['seed'],k['seat']):i for i,k in enumerate(keys)}
products = ['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON','EGG','MILK','WOOL','FERTILIZER']
rows = []
for opponent in ['empty_sale_slots_m2','public_router']:
    for actor in ['service_bank_p362_m0','service_bank_p362_m2']:
        games = json.loads((EXP/'runs/day_service_bank_sep08_001/fresh'/f'{actor}_vs_{opponent}.json').read_text())['games']
        indices = [lookup[actor,opponent,g['seed'],g['seat']] for g in games]
        views = [model['observed_lifetimes'][i] for i in indices]
        average = lambda values: statistics.mean(values)
        row = {'actor':actor,'opponent':opponent,'games':len(games),
            'cash':average(g['cash'] for g in games),'rival_cash':average(g['opponent_cash'] for g in games),
            'realized_output':[average(g['produced'][i] for g in games) for i in range(9)],
            'realized_discards':[average(g['discarded'][i] for g in games) for i in range(9)],
            'hire_cost':average(g['profile']['hire_cost'] for g in games),
            'animal_birth_day':{str(item):average(l[5] for g in games for l in g['profile']['lives'] if l[0]==item) for item in [9,10,11]},
            'conditional_forecasts':[]}
        for key in ['standard_service','all_crops_fertilized','observed_animal_service_daily_harvest','active_days','births']:
            row[key] = [average(v[key][i] for v in views) for i in range(len(views[0][key]))]
        for intended in model['intended']:
            forecasts = [intended['scenario_forecasts'][i] for i in indices]
            row['conditional_forecasts'].append({'mode':intended['mode'],
                'cash':average(f['cash'] for f in forecasts),
                'margin':average(f['cash']-f['rival_cash'] for f in forecasts),
                'min_cash':average(f['min_cash'] for f in forecasts),
                'funding_deficit_games':sum(f['min_cash']<0 for f in forecasts),
                'missing_input_games':sum(f['first_input_gap_step']>=0 for f in forecasts)})
        if actor=='service_bank_p362_m2':
            reverse = json.loads((RUN/'reverse'/f'{opponent}_vs_{actor}.json').read_text())['games']
            row['rival_realized_output'] = [average(g['produced'][i] for g in reverse) for i in range(9)]
            row['rival_hire_cost'] = average(g['profile']['hire_cost'] for g in reverse)
        rows.append(row)
intended = [{k:v for k,v in p.items() if k!='scenario_forecasts'} for p in model['intended']]
report = {'profile_models':len(keys),'exact_reversed_games':512,'intended':intended,'rows':rows,
    'scope':['All comparisons use the same frozen seed2300000..2300127 games/both seats. Reversed runs reproduce both action hashes, cash and profiles exactly.',
        'Standard service uses fertilizer on ongoing crops only, productive one-shot harvest, and daily animal feed/care/collect/harvest. All-crops-fertilized changes one-shot service too.',
        'Observed lifetimes retain actual births and end dates, including losses/delays. Their ideal service is a conditional dated model, not a feasible schedule or certified bound.',
        'Observed animal service retains recorded successful feed/care/collect day masks but assumes daily harvest; it cannot reconstruct exact intra-day harvest/cap losses.',
        'Financial forecasts retain fixed opponent flows from these games, original intended support, heuristic deposit times, approximate order interleaving and visible funding/input deficits. They do not predict opponent policy responses or demonstrate feasible cash.'],
    'decision':'Prioritize composition renewal and funding alongside service compilation. Do not extrapolate one solved day into a competitive cold farm.'}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text = '# Cold-farm composition, placement and service gap\n\nThe original p362 output and212-turn work deficit reproduce exactly. This diagnosis uses1024 saved profiles and512 reversed full games; every reversed cash/action hash/profile matches the original game.\n'
for r in rows:
    if r['actor']!='service_bank_p362_m2':
        continue
    text += f"\nAgainst {r['opponent']}: cold cash {r['cash']:.2f}, opponent {r['rival_cash']:.2f}; hire costs {r['hire_cost']:.2f} versus {r['rival_hire_cost']:.2f}.\n\n| Product | Intended standard | Intended fertilized | Actual lifetimes, standard service | Realized cold output | Realized opponent output |\n| --- | ---: | ---: | ---: | ---: | ---: |\n"
    for i,item in enumerate(products):
        text += f"| {item} | {intended[0]['produced'][i]} | {intended[1]['produced'][i]} | {r['standard_service'][i]:.2f} | {r['realized_output'][i]:.2f} | {r['rival_realized_output'][i]:.2f} |\n"
    text += '\nConditional intended-plan economic forecasts on the same observed rival flows:\n'
    for f in r['conditional_forecasts']:
        text += f"- Mode {f['mode']}: cash {f['cash']:.2f}, margin {f['margin']:.2f}, minimum cash {f['min_cash']:.2f}; funding deficits in {f['funding_deficit_games']}/256, missing inputs in {f['missing_input_games']}/256.\n"
text += '\nInterpretation limits:\n'+''.join('- '+s+'\n' for s in report['scope'])
(RUN/'RESULTS.md').write_text(text)
print(text)
