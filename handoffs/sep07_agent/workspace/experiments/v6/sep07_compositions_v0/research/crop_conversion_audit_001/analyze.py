"""Offline crop-instance and transaction audit of selected public replay cohorts."""
import csv
import hashlib
import json
import statistics
from collections import Counter, defaultdict
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
COHORTS = ['refresh_1212', 'refresh_1112']
CROPS = ['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON']
ITEMS = CROPS+['EGG','MILK','WOOL','FERTILIZER']
BONUS = [{2,3,4},{2,3},{7,8,9,10},{9,11,13,15},set(range(6,13))]
SEED_COST = [10,20,50,100,80]
LOCAL = EXP / 'results/investment_context_checks/investment_context_guarded_001_best_profile64.json'


def read_csv(path):
    return list(csv.DictReader(path.open()))


def write_csv(name, rows):
    if not rows:
        return
    with (RUN / name).open('w') as out:
        writer = csv.DictWriter(out,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)


def main():
    hashes = {str(LOCAL.relative_to(EXP)): hashlib.sha256(LOCAL.read_bytes()).hexdigest()}
    groups, instances, plans, transactions = {}, [], [], []
    for cohort in COHORTS:
        base = EXP / 'research' / cohort
        data = {}
        for name in ['top_replay_manifest','game_summary','crop_instances','crop_days','unit_events','transactions']:
            path = base / f'{name}.csv';hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest();data[name] = read_csv(path)
        metadata = {(r['episode_id'],r['team']):r for r in data['top_replay_manifest']}
        summaries = {(r['episode_id'],r['team']):r for r in data['game_summary']}
        events = defaultdict(list)
        for index,row in enumerate(data['unit_events']):
            row = dict(row);row['event_index']=index
            for name in ['result_index','day','hour','x','y','quantity']:
                row[name]=int(row[name])
            events[row['episode_id'],row['team']].append(row)
        per_game_days, per_game_trans = defaultdict(list), defaultdict(list)
        for row in data['crop_days']:
            per_game_days[row['episode_id'],row['team']].append(row)
        for row in data['transactions']:
            per_game_trans[row['episode_id'],row['team']].append(row)
        for key, meta in metadata.items():
            episode,team=key;summary=summaries[key];ev=events[key]
            selected = [r for r in data['crop_instances'] if (r['episode_id'],r['team'])==key]
            group = {'cohort':cohort,'episode':int(episode),'team':team,'rank':int(meta['rank']),'submission':int(meta['submission_id']),
                     'leaderboard_score':float(meta['leaderboard_score']),'seat':int(summary['seat']),'cash':float(summary['reward']),
                     'crop':{},'fertilizer_collected':sum(e['quantity'] for e in ev if e['operation']=='COLLECT_FERTILIZER'),
                     'fertilizer_applied':sum(e['operation']=='FERTILIZE' for e in ev),'transactions':per_game_trans[key]}
            for crop in CROPS:
                cropdays=[d for d in per_game_days[key] if d['crop']==crop]
                relevant=[d for d in cropdays if int(d['yield_relevant']) and not (crop in ['TOMATO','STRAWBERRY'] and int(d['day'])==29)]
                crop_events=[e for e in ev if e['source']==crop]
                trades=[t for t in per_game_trans[key] if t['item']==crop]
                group['crop'][crop]={'instances':sum(r['name']==crop for r in selected),'days':len(cropdays),
                    'water_days':sum(int(d['watered']) for d in cropdays),'relevant_days':len(relevant),
                    'maximized_days':sum(int(d['maximized']) for d in relevant),'harvested':sum(e['quantity'] for e in crop_events if e['operation']=='HARVEST'),
                    'fertilizer_applications':sum(e['operation']=='FERTILIZE' for e in crop_events),
                    'bought_product':sum(int(t['actual']) for t in trades if t['operation']=='BUY_PRODUCT'),
                    'sold_product':sum(int(t['actual']) for t in trades if t['operation']=='SELL'),
                    'sale_revenue':sum(float(t['value']) for t in trades if t['operation']=='SELL'),
                    'product_spend':sum(float(t['value']) for t in trades if t['operation']=='BUY_PRODUCT'),
                    'seeds_bought':sum(int(t['actual']) for t in trades if t['operation']=='BUY_SEED'),
                    'seed_spend':sum(float(t['value']) for t in trades if t['operation']=='BUY_SEED')}
            ft=[t for t in per_game_trans[key] if t['item']=='FERTILIZER']
            for side,op in [('bought','BUY_PRODUCT'),('sold','SELL')]:
                group[f'fertilizer_{side}']=sum(int(t['actual']) for t in ft if t['operation']==op)
                group[f'fertilizer_{side}_value']=sum(float(t['value']) for t in ft if t['operation']==op)
            groups[cohort,episode,team]=group
            for item in selected:
                x,y,start,end=map(int,[item['x'],item['y'],item['start_state'],item['end_state']]);crop=item['name'];born=int(item['origin_day'])
                service=[e for e in ev if e['x']==x and e['y']==y and start<=e['result_index']<=end and e['source']==crop and e['operation'] in ['PLANT','WATER','FERTILIZE','HARVEST','DIG']]
                fert=[e for e in service if e['operation']=='FERTILIZE'];harvest=[e for e in service if e['operation']=='HARVEST'];water=[e for e in service if e['operation']=='WATER']
                crop_id=CROPS.index(crop);row={'cohort':cohort,'episode':int(episode),'team':team,'rank':int(meta['rank']),'submission':int(meta['submission_id']),
                    'seat':int(summary['seat']),'crop':crop,'x':x,'y':y,'plant_day':born,'start_state':start,'end_state':end,
                    'end_day':int(item['end_day']),'outcome':item['outcome'],'harvested':sum(e['quantity'] for e in harvest),
                    'seed_cost':SEED_COST[crop_id],'fertilizer_applications':len(fert),'water_visits':len(water),
                    'water_days':[e['day'] for e in water],'fertilizer_steps':[e['result_index']-1 for e in fert],
                    'harvest_steps_and_quantity':[[e['result_index']-1,e['quantity']] for e in harvest],
                    'service_events':service}
                instances.append(row)
            for t in per_game_trans[key]:transactions.append({'cohort':cohort,'seat':int(summary['seat']),'submission':int(meta['submission_id']),**t})
    local=json.loads(LOCAL.read_text());local_rows=[]
    for game in local['games']:
        profile=game['profile'];group={'cohort':'local_reference','episode':game['seed'],'team':'investment_context_guarded_001_best','rank':None,'submission':None,'seat':game['seat'],
             'cash':game['cash'],'crop':{},'fertilizer_collected':game['produced'][8],'fertilizer_applied':profile['successful'][11],
             'fertilizer_bought':profile['buys'][8],'fertilizer_sold':game['sold'][8]}
        for crop_id,crop in enumerate(CROPS):
            lives=[l for l in profile['lives'] if l[0]==crop_id];relevant=maximized=0
            for life in lives:
                mask=sum(1<<d for d in range(30) if d-life[5] in BONUS[crop_id] and not (crop_id in [2,3] and d==29))&life[6]
                relevant+=mask.bit_count();maximized+=(mask&life[7]&life[11]).bit_count()
                local_rows.append({'seed':game['seed'],'seat':game['seat'],'crop':crop,'x':life[1],'y':life[2],'start_state':life[3],'end_state':life[4],'plant_day':life[5],
                     'days':[d for d in range(30) if life[6]&(1<<d)],'water_days':[d for d in range(30) if life[7]&(1<<d)],
                     'fertilizer_active_days':[d for d in range(30) if life[11]&(1<<d)],'relevant_days':[d for d in range(30) if mask&(1<<d)]})
            group['crop'][crop]={'instances':len(lives),'days':sum(l[6].bit_count() for l in lives),'water_days':sum(l[7].bit_count() for l in lives),
                  'relevant_days':relevant,'maximized_days':maximized,'harvested':game['produced'][crop_id],
                  'bought_product':profile['buys'][crop_id],'sold_product':game['sold'][crop_id], 'seeds_bought':profile['seed_buys'][crop_id],
                  'seed_spend':profile['seed_buys'][crop_id]*SEED_COST[crop_id]}
        groups['local_reference',str(game['seed']),str(game['seat'])]=group
    # Per-crop comparisons avoid attributing a different crop mix to better service.
    comparisons=[]
    for team in sorted({g['team'] for g in groups.values()}):
        games=[g for g in groups.values() if g['team']==team]
        for crop in CROPS:
            sums=Counter()
            for g in games:sums.update(g['crop'][crop])
            comparisons.append({'team':team,'crop':crop,'games':len(games),'mean_instances':sums['instances']/len(games),'mean_crop_days':sums['days']/len(games),
                'mean_harvested':sums['harvested']/len(games),'output_per_planted_seed':sums['harvested']/sums['instances'] if sums['instances'] else None,
                'output_per_crop_day':sums['harvested']/sums['days'] if sums['days'] else None,
                'water_rate':sums['water_days']/sums['days'] if sums['days'] else None,
                'relevant_days':sums['relevant_days'],'maximized_days':sums['maximized_days'],
                'yield_day_fertilized_and_watered':sums['maximized_days']/sums['relevant_days'] if sums['relevant_days'] else None,
                'mean_fertilizer_applications':sums['fertilizer_applications']/len(games) if team!='investment_context_guarded_001_best' else None})
    econ=[]
    for g in groups.values():econ.append({k:v for k,v in g.items() if k not in ['crop','transactions']})
    for name,value in [('games',list(groups.values())),('crop_instances',instances),('local_instances',local_rows)]:
        (RUN/f'{name}.json').write_text(json.dumps(value,indent=2)+'\n')
    write_csv('crop_comparison.csv',comparisons);write_csv('fertilizer_economics.csv',econ);write_csv('transactions.csv',transactions)
    (RUN/'SOURCE_HASHES.json').write_text(json.dumps(hashes,indent=2)+'\n')
    print(json.dumps(comparisons,indent=2))


if __name__=='__main__':main()
