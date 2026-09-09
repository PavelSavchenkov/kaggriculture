"""Offline source-exact service calendars; no forecast profitability claims."""
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]


def rows(name):
    return list(csv.DictReader((RUN / name).open()))


def main():
    metadata = {(r['episode_id'], r['team']): r for r in rows('top_replay_manifest.csv')}
    events = defaultdict(list)
    for r in rows('unit_events.csv'):
        if r['origin_day']:
            events[r['episode_id'], r['team'], r['x'], r['y'], r['source'], r['origin_day']].append(r)
    records, cells = [], defaultdict(list)
    for r in rows('crop_instances.csv'):
        ev = events[r['episode_id'], r['team'], r['x'], r['y'], r['name'], r['origin_day']]
        ev = [x for x in ev if int(r['start_state']) <= int(x['result_index']) <= int(r['end_state'])]
        schedule = {op: [[int(x['result_index'])-1, int(x['day']), int(x['hour']), int(x['quantity'])] for x in ev if x['operation']==op] for op in ['WATER','FERTILIZE','HARVEST','DIG']}
        rec = {'episode':int(r['episode_id']),'team':r['team'],'submission':int(metadata[r['episode_id'],r['team']]['submission_id']), 'x':int(r['x']),'y':int(r['y']),'crop':r['name'],'plant_day':int(r['origin_day']),'start_state':int(r['start_state']),'end_state':int(r['end_state']),'end_day':int(r['end_day']), 'outcome':r['outcome'],'harvest':sum(x[3] for x in schedule['HARVEST']),'schedule':schedule}
        records.append(rec);cells[r['episode_id'],r['team'],r['x'],r['y']].append(rec)
    summary=[]
    for team in sorted({r['team']for r in records}):
        for crop in ['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON']:
            selected=[r for r in records if r['team']==team and r['crop']==crop]
            if not selected:continue
            templates=Counter((r['end_day']-r['plant_day'],r['harvest'],tuple(x[1]-r['plant_day']for x in r['schedule']['WATER']),tuple(x[1]-r['plant_day']for x in r['schedule']['FERTILIZE']))for r in selected)
            summary.append({'team':team,'crop':crop,'instances':len(selected),'harvest_per_seed':sum(r['harvest']for r in selected)/len(selected),'top_templates':[{'lifetime_days':k[0],'harvest':k[1],'water_ages':k[2],'fertilizer_ages':k[3],'instances':n}for k,n in templates.most_common(5)]})
    examples=[]
    wanted=[('get some fries','WHEAT',6),('get some fries','MELON',6),('3정훈','TOMATO',8)]
    for team,crop,harvest in wanted:
        selected=next(r for r in records if r['team']==team and r['crop']==crop and r['harvest']==harvest and r['plant_day']>=3)
        key=(str(selected['episode']),team,str(selected['x']),str(selected['y']))
        source=EXP/'replays'/f"episode-{selected['episode']}-replay.json";replay=json.loads(source.read_text());seat=replay['info']['TeamNames'].index(team)
        chain=sorted(cells[key],key=lambda r:r['start_state'])
        days=sorted({d for r in chain for d in range(r['plant_day'],r['end_day']+1)})
        example={'selected':selected,'cell_rotation':chain,'seat':seat,'replay_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'complete_day_actions':{str(d):[replay['steps'][t+1][seat]['action']for t in range(d*24,min((d+1)*24,719))]for d in days},'scope':'Observed donor course only; surrounding inputs, land, workers, deposits and sales require complete recompilation. No private code or marginal profit inferred.'}
        examples.append(example)
    glob=json.loads((RUN/'invariant_means.json').read_text())['means'];local=json.loads((EXP/'results/investment_context_checks/investment_context_guarded_001_best_profile64.profile.json').read_text())['means']
    comparison=[]
    for key,value in glob.items():
        ownkey='produced_'+key.removeprefix('harvest_')if key.startswith('harvest_')else'faults'if key=='unit_faults'else key
        if ownkey in local:comparison.append({'metric':key,'global72':value,'local64':local[ownkey]})
    for name,data in [('crop_lifecycles',records),('crop_service_templates',summary),('larger_crop_examples',examples),('reference_comparison',comparison)]:
        (RUN/f'{name}.json').write_text(json.dumps(data,indent=2,ensure_ascii=False)+'\n')
    print(json.dumps([{'selected':x['selected'],'rotation':[(r['crop'],r['plant_day'],r['end_day'],r['harvest'])for r in x['cell_rotation']]}for x in examples],ensure_ascii=False),flush=True)


if __name__=='__main__':main()
