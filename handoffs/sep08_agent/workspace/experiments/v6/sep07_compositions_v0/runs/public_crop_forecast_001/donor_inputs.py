"""Extract public current crops and later observed outcomes as separate data."""
import csv
import hashlib
import json
from collections import defaultdict
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
SOURCE=EXP/'research/refresh_1342'
ITEMS='WHEAT CARROT TOMATO STRAWBERRY MELON EGG MILK WOOL FERTILIZER GOOSE COW SHEEP'.split()
KINDS={'PLANT':5,'COOP':3,'PASTURE':4}


def grid(n):return [[0]*n for _ in range(30)]


def main():
    lives=defaultdict(list);transactions=defaultdict(list);metadata=defaultdict(list)
    for row in json.loads((SOURCE/'crop_lifecycles.json').read_text()):lives[row['episode'],row['team']].append(row)
    for row in csv.DictReader((SOURCE/'transactions.csv').open()):transactions[int(row['episode_id']),row['team']].append(row)
    for row in csv.DictReader((SOURCE/'top_replay_manifest.csv').open()):metadata[int(row['episode_id'])].append(row)
    cases=[];inputs=[];hashes={}
    for episode,teams in sorted(metadata.items()):
        path=EXP/f'replays/episode-{episode}-replay.json';data=path.read_bytes();hashes[str(path.relative_to(EXP))]=hashlib.sha256(data).hexdigest();replay=json.loads(data)
        for meta in teams:
            team=meta['team'];seat=replay['info']['TeamNames'].index(team);records=lives[episode,team]
            for step in [226,264,288,360,432,504,576]:
                obs=replay['steps'][step][seat]['observation'];farm=obs['farms'][seat];tiles=[];active=[]
                for y,row in enumerate(farm['tiles']):
                    for x,t in enumerate(row):
                        if not isinstance(t,dict)or not(t.get('crop')or t.get('animal')):continue
                        animal=t.get('animal');what=ITEMS.index(t.get('crop')or animal)
                        values=[x,y,KINDS[t['kind']],what,int(bool(animal)),int(t.get('watered_today',False)),int(t.get('fed_today',False)),int(t.get('cared_today',False)),int(t.get('fertilizer_available',False)),t.get('consecutive_unwatered',t.get('consecutive_unfed',0)),t.get('yield_units',0),t.get('pending_care_bonus',0),t.get('planted_day',t.get('placed_day',0)),t.get('max_lifespan_step',-1),t.get('fertilized_until_day',-1)];tiles.append(values)
                        if t.get('crop'):
                            match=[r for r in records if r['x']==x and r['y']==y and r['crop']==t['crop'] and r['plant_day']==t['planted_day'] and r['start_state']<=step<r['end_state']]
                            assert len(match)==1,(episode,team,step,x,y,t,match);active.append(match[0])
                current=grid(5);all_future=grid(5);sales=grid(9);buys=grid(9)
                for r in records:
                    for at,day,hour,n in r['schedule']['HARVEST']:
                        if at>=step:all_future[day][ITEMS.index(r['crop'])]+=n
                for r in active:
                    for at,day,hour,n in r['schedule']['HARVEST']:
                        if at>=step:current[day][ITEMS.index(r['crop'])]+=n
                for r in transactions[episode,team]:
                    if int(r['day'])*24+int(r['hour'])<step:continue
                    if r['operation']=='SELL':sales[int(r['day'])][ITEMS.index(r['item'])]+=int(r['actual'])
                    if r['operation']=='BUY_PRODUCT':buys[int(r['day'])][ITEMS.index(r['item'])]+=int(r['actual'])
                cases.append({'case':len(cases),'episode':episode,'team':team,'seat':seat,'submission':int(meta['submission_id']),'rank':int(meta['rank']),'step':step,'day':obs['day'],'hour':obs['hour'],'visible_current_crops':len(active),'current_crop_future_harvest':current,'all_crop_future_harvest':all_future,'future_sales':sales,'future_buys':buys})
                inputs.append([step,obs['day'],obs['hour'],len(tiles),*[v for tile in tiles for v in tile]])
        print('extracted',episode,len(cases),'contexts',flush=True)
    with (RUN/'donor_inputs.txt').open('w')as f:
        f.write(str(len(inputs))+'\n')
        for row in inputs:f.write(' '.join(map(str,row))+'\n')
    (RUN/'donor_cases.json').write_text(json.dumps(cases,separators=(',',':'),ensure_ascii=False)+'\n')
    for p in [SOURCE/'crop_lifecycles.json',SOURCE/'transactions.csv',SOURCE/'top_replay_manifest.csv']:hashes[str(p.relative_to(EXP))]=hashlib.sha256(p.read_bytes()).hexdigest()
    (RUN/'DONOR_INPUT_LINEAGE.json').write_text(json.dumps({'source_sha256':hashes,'cases':len(cases),'source':'Global top12cohort selected13:43UTC; six recent games per player','boundary':'Only current public crop/animal tile fields enter donor_inputs.txt. Later observed harvest/trade outcomes remain in donor_cases.json for offline evaluation only. No private donor code or future runtime inputs.'},indent=2)+'\n')


if __name__=='__main__':main()
