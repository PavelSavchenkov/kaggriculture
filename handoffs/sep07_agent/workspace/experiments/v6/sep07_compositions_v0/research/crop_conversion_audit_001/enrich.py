"""Read source observations for fertilizer certificates and full cash balances."""
import csv
import hashlib
import json
from collections import Counter,defaultdict
from pathlib import Path

from analyze import BONUS,CROPS,EXP,RUN


def main():
    instances=json.loads((RUN/'crop_instances.json').read_text());games=json.loads((RUN/'games.json').read_text())
    by_episode=defaultdict(list)
    for game in games:
        if game['cohort']!='local_reference':by_episode[game['episode']].append(game)
    instances_by_game=defaultdict(list)
    for instance in instances:instances_by_game[instance['episode'],instance['team']].append(instance)
    hashes={};economics=[];applications=[]
    for episode, selected in by_episode.items():
        path=EXP/'replays'/f'episode-{episode}-replay.json';raw=path.read_bytes();hashes[str(path.relative_to(EXP))]=hashlib.sha256(raw).hexdigest();replay=json.loads(raw);del raw
        for game in selected:
            seat=game['seat'];first=replay['steps'][0][seat]['observation'];last=replay['steps'][-1][seat]['observation']
            assert replay['info']['TeamNames'][seat]==game['team']
            portable=lambda observation,item:observation['private']['shed'].get(item,0)+sum(inv.get(item,0)for inv in observation['private']['inventories'])
            tx=game['transactions'];revenue=sum(float(t['value'])for t in tx if t['operation']=='SELL');spend=sum(float(t['value'])for t in tx if t['operation']!='SELL')
            initial=first['farms'][seat]['money'];assert initial+revenue-spend==game['cash'],(episode,game['team'])
            fertilizer_loss=portable(first,'FERTILIZER')+game['fertilizer_collected']+game['fertilizer_bought']-game['fertilizer_applied']-game['fertilizer_sold']-portable(last,'FERTILIZER')
            assert fertilizer_loss>=0
            economics.append({k:game[k]for k in ['cohort','episode','team','rank','submission','seat','cash','fertilizer_collected','fertilizer_bought','fertilizer_applied','fertilizer_sold']}|
                {'replay_sha256':hashes[str(path.relative_to(EXP))],'initial_cash':initial,'sale_revenue':revenue,'total_spend':spend,
                 'spend_by_operation':dict(Counter({op:sum(float(t['value'])for t in tx if t['operation']==op)for op in sorted({t['operation']for t in tx if t['operation']!='SELL'})})),
                 'initial_fertilizer':portable(first,'FERTILIZER'),'final_fertilizer':portable(last,'FERTILIZER'),'fertilizer_discard_balance':fertilizer_loss,'crop_economics':game['crop']})
            for item in instances_by_game[episode,game['team']]:
                item['replay_sha256']=hashes[str(path.relative_to(EXP))];item['fertilizer_quote_opportunity_cost']=0;item['harvest_quote_value']=0
                for step,quantity in item['harvest_steps_and_quantity']:
                    item['harvest_quote_value']+=quantity*replay['steps'][step][seat]['observation']['market']['prices'][item['crop']]
                events=item['service_events'];fert=[e for e in events if e['operation']=='FERTILIZE'];water=[e for e in events if e['operation']=='WATER'];previous_same_step={}
                for event in fert:
                    step=event['result_index']-1;observation=replay['steps'][step][seat]['observation'];tile=observation['farms'][seat]['tiles'][item['y']][item['x']]
                    before=tile.get('fertilized_until_day',-1) if isinstance(tile,dict)and tile.get('kind')=='PLANT'and tile.get('crop')==item['crop'] else -1
                    before=max(before,previous_same_step.get(step,-1));previous_same_step[step]=max(before,event['day']+2)
                    quote=observation['market']['prices']['FERTILIZER'];item['fertilizer_quote_opportunity_cost']+=quote
                    if item['crop']in ['TOMATO','STRAWBERRY']:
                        opportunities=[{'day':day,'step':(day+1)*24-1}for day in sorted(set(e['day']for e in water))
                            if day-item['plant_day']in BONUS[CROPS.index(item['crop'])]and day<29 and item['end_state']>=(day+1)*24
                            and step<=(day+1)*24-1 and day<=event['day']+2]
                    else:
                        opportunities=[{'day':e['day'],'step':e['result_index']-1}for e in water if e['day']-item['plant_day']in BONUS[CROPS.index(item['crop'])]
                            and e['event_index']>event['event_index']and e['day']<=event['day']+2]
                    extra=[e for e in opportunities if e['day']>before]
                    applications.append({'cohort':item['cohort'],'episode':episode,'team':game['team'],'submission':game['submission'],'seat':seat,
                         'crop':item['crop'],'x':item['x'],'y':item['y'],'plant_day':item['plant_day'],'step':step,'day':event['day'],
                         'previous_fertilized_until':before,'new_fertilized_until':max(before,event['day']+2),'quote':quote,
                         'duplicates_existing_expiry':before>=event['day']+2,'no_future_productive_water_or_update':not opportunities,
                         'no_added_productive_opportunity_over_existing_coverage':not extra,'protected_opportunities':opportunities,
                         'extra_opportunities':extra,'replay_sha256':item['replay_sha256']})
        del replay
    for name,value in [('enriched_instances',instances),('full_observed_economics',economics),('fertilizer_applications',applications),('REPLAY_HASHES',hashes)]:
        (RUN/f'{name}.json').write_text(json.dumps(value,indent=2)+'\n')
    summary={}
    for team in sorted({a['team']for a in applications}):
        rows=[a for a in applications if a['team']==team]
        summary[team]={'applications':len(rows),**{key:sum(a[key]for a in rows)for key in ['duplicates_existing_expiry','no_future_productive_water_or_update','no_added_productive_opportunity_over_existing_coverage']}}
    (RUN/'APPLICATION_SUMMARY.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(summary,indent=2));print(f'{len(economics)} exact cash and fertilizer balances checked.')


if __name__=='__main__':main()
