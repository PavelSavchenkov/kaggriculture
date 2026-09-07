"""Export concrete donor/local crop blocks and transparent price-path diagnostics."""
import csv
import hashlib
import json
import statistics
from collections import Counter,defaultdict

from analyze import CROPS,EXP,RUN,SEED_COST,BONUS,read_csv,write_csv

OPS={8:'PLANT',9:'WATER',10:'HARVEST',11:'FERTILIZE',12:'DIG'}


def main():
    donors=json.loads((RUN/'enriched_instances.json').read_text());local=json.loads((RUN/'local_instances.json').read_text());traces=json.loads((RUN/'local_events.json').read_text())
    events={(g['seed'],g['seat']):g['events']for g in traces};local_enriched=[]
    for life in local:
        if (life['seed'],life['seat'])not in events:continue
        item=CROPS.index(life['crop']);selected=[e for e in events[life['seed'],life['seat']]if e[3]==item and e[4]==life['x']and e[5]==life['y']and e[6]==life['plant_day']and life['start_state']-1<=e[0]<life['end_state']]
        life={**life,'events':selected,'harvested':sum(e[7]for e in selected if e[2]==10),'seed_cost':SEED_COST[item],
              'fertilizer_applications':sum(e[2]==11 for e in selected),'harvest_steps_and_quantity':[[e[0],e[7]]for e in selected if e[2]==10]}
        local_enriched.append(life)
    (RUN/'local_enriched_instances.json').write_text(json.dumps(local_enriched,indent=2)+'\n')
    specs=[
        {'id':'strawberry_day20_completion','episode':106446230,'team':'3정훈','x':9,'y':3,'start_day':7,'end_day':24,'crop':'STRAWBERRY','rank':1},
        {'id':'wheat_day14_conversion','episode':106443319,'team':'ymg_aq','x':3,'y':0,'start_day':12,'end_day':16,'crop':'WHEAT','rank':2},
        {'id':'tomato_rotation_after_day9','episode':106429645,'team':'Mengfei Li','x':0,'y':1,'start_day':9,'end_day':29,'crop':None,'rank':3},
        {'id':'carrot_day28_conversion','episode':106443319,'team':'ymg_aq','x':8,'y':3,'start_day':26,'end_day':29,'crop':'CARROT','rank':4}]
    proposals=[]
    for spec in specs:
        def selected(life,external):
            match=(life['episode']==spec['episode']and life['team']==spec['team'])if external else(life['seed']==1000 and life['seat']==0)
            return match and life['x']==spec['x']and life['y']==spec['y']and spec['start_day']<=life['plant_day']<=spec['end_day']and(spec['crop']is None or life['crop']==spec['crop'])
        donor=[x for x in donors if selected(x,True)];own=[x for x in local_enriched if selected(x,False)];assert donor and own
        path=EXP/'replays'/f'episode-{spec["episode"]}-replay.json';replay=json.loads(path.read_bytes());seat=donor[0]['seat']
        quote=lambda step,product:replay['steps'][step][seat]['observation']['market']['prices'][product]
        def totals(lives,external):
            harvest=Counter();operations=Counter();value=0;fert_value=0
            for life in lives:
                harvest[life['crop']]+=life['harvested']
                for step,quantity in life['harvest_steps_and_quantity']:value+=quantity*quote(step,life['crop'])
                if external:
                    operations.update(e['operation']for e in life['service_events']);fert_steps=life['fertilizer_steps']
                else:
                    operations.update(OPS[e[2]]for e in life['events']);fert_steps=[e[0]for e in life['events']if e[2]==11]
                fert_value+=sum(quote(step,'FERTILIZER')for step in fert_steps)
            seeds=sum(l['seed_cost']for l in lives)
            return {'harvested':dict(harvest),'seed_cost':seeds,'fertilizer_units':operations['FERTILIZE'],'unit_operations':dict(operations),
                    'harvest_quote_value_on_donor_price_path':value,'fertilizer_quote_opportunity_cost_on_donor_path':fert_value,
                    'shadow_contribution_before_labor_land_and_market_feedback':value-seeds-fert_value}
        dt,lt=totals(donor,True),totals(own,False)
        directory=RUN/'proposals'/spec['id'];directory.mkdir(parents=True,exist_ok=True)
        cohort=donor[0]['cohort'];full_events=read_csv(EXP/'research'/cohort/'unit_events.csv')
        tile_events=[e for e in full_events if int(e['episode_id'])==spec['episode']and e['team']==spec['team']and int(e['x'])==spec['x']and int(e['y'])==spec['y']and spec['start_day']<=int(e['day'])<=spec['end_day']]
        write_path=directory/'donor_tile_events.json';write_path.write_text(json.dumps(tile_events,indent=2)+'\n')
        day_actions={str(day):[replay['steps'][step+1][seat].get('action') for step in range(day*24,min((day+1)*24,719))]for day in range(spec['start_day'],spec['end_day']+1)}
        (directory/'donor_complete_day_actions.json').write_text(json.dumps(day_actions,indent=2)+'\n')
        (directory/'donor_lifecycles.json').write_text(json.dumps(donor,indent=2)+'\n');(directory/'local_lifecycles.json').write_text(json.dumps(own,indent=2)+'\n')
        similar=[x for x in donors if x['team']==spec['team']and x['crop']==('TOMATO'if spec['crop']is None else spec['crop'])and x['fertilizer_applications']==(2 if spec['crop']in [None,'STRAWBERRY']else 1)
                 and x['harvested']==(8 if spec['crop']in [None,'STRAWBERRY']else 6 if spec['crop']=='WHEAT'else 4)]
        dependencies=['Rebuild fertilizer purchase/reserve/pickup and carried inventory constraints; animal-supplied fertilizer still has sale opportunity cost.',
             'Compile complete affected worker days with seed, water, fertilizer, harvest, drop and sale jobs; rebind downstream physical-state guards.',
             'Revalue all own product sales and rival revenue under changed market supply; enforce sale capacity and terminal liquidation.',
             'Check every retained day until the affected crop leaves the bed, not just the edited fertilizer day.']
        if spec['crop']is None:dependencies+=['Replace the whole cell suffix from day9: crop occupancy, four wheat replants, tomato seed purchase, two fertilizer services, four tomato harvests, weed clearing and final carrot.',
             'Replace lost dated wheat delivery with purchases or other production before animal feed is due; verify cash and shed/carry capacity.',
             'Add tomato-family market orders and timed output; preserve land ownership and animal structures on other cells.',
             'Donor full-day actions are evidence, not directly transferable schedules: its other crops, animals, workers and inventories differ.']
        proposal={**spec,'status':'Uncompiled replay-derived proposal; profitability not proven','donor_seat':seat,'submission':donor[0]['submission'],
             'leaderboard_rank_at_selection':donor[0]['rank'],'replay_sha256':donor[0]['replay_sha256'],
             'local_match':{'agent':'investment_context_guarded_001_best','seed':1000,'seat':0,'opponent':'public_router'},
             'donor_totals':dt,'local_totals':lt,'harvest_delta':{crop:dt['harvested'].get(crop,0)-lt['harvested'].get(crop,0)for crop in CROPS},
             'seed_cost_delta':dt['seed_cost']-lt['seed_cost'],'fertilizer_units_delta':dt['fertilizer_units']-lt['fertilizer_units'],
             'unit_operation_delta':{op:dt['unit_operations'].get(op,0)-lt['unit_operations'].get(op,0)for op in ['PLANT','WATER','HARVEST','FERTILIZE','DIG']},
             'shadow_contribution_delta':dt['shadow_contribution_before_labor_land_and_market_feedback']-lt['shadow_contribution_before_labor_land_and_market_feedback'],
             'valuation_limit':'Diagnostic values both schedules at donor contemporaneous harvest quotes and fertilizer opportunity quotes. This is not realized crop-specific cash, marginal market pricing, or a profit claim.',
             'matching_species_output_service_examples':len(similar),'distinct_donor_episodes':len({x['episode']for x in similar}),
             'implementation_dependencies':dependencies,
             'artifact_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in directory.iterdir()if p.is_file()}}
        (directory/'PROPOSAL.json').write_text(json.dumps(proposal,indent=2)+'\n');proposals.append(proposal)
    rows=read_csv(RUN/'crop_comparison.csv');local_rates={r['crop']:r for r in rows if r['team']=='investment_context_guarded_001_best'}
    comparisons=[]
    local_total=sum(int(r['relevant_days'])for r in local_rates.values())
    local_rate=sum(int(r['maximized_days'])for r in local_rates.values())/local_total
    for team in sorted({r['team']for r in rows if r['team']!='investment_context_guarded_001_best'}):
        values={r['crop']:r for r in rows if r['team']==team};total=sum(int(r['relevant_days'])for r in values.values());actual=sum(int(r['maximized_days'])for r in values.values())/total
        within=mix=0
        for crop in CROPS:
            l,t=local_rates[crop],values[crop];wl=int(l['relevant_days'])/local_total;wt=int(t['relevant_days'])/total
            rl=float(l['yield_day_fertilized_and_watered']or 0);rt=float(t['yield_day_fertilized_and_watered']or 0)
            within+=wl*(rt-rl);mix+=(wt-wl)*rt
        assert abs(actual-local_rate-within-mix)<1e-10
        comparisons.append({'team':team,'local_rate':local_rate,'donor_rate':actual,'within_crop_gap_at_local_mix':within,'crop_mix_term':mix})
    write_csv('mix_decomposition.csv',comparisons)
    (RUN/'PROPOSALS.json').write_text(json.dumps(proposals,indent=2)+'\n')
    print(json.dumps([{k:p[k]for k in ['id','harvest_delta','seed_cost_delta','fertilizer_units_delta','unit_operation_delta','shadow_contribution_delta','matching_species_output_service_examples','distinct_donor_episodes']}for p in proposals],indent=2))
    print(json.dumps(comparisons,indent=2))


if __name__=='__main__':main()
