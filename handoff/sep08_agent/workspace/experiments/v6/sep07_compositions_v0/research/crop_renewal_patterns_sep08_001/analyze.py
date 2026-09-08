from collections import defaultdict
from pathlib import Path
import csv
import hashlib
import json
import statistics

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
SOURCE=EXP/'research/refresh_sep08_0708'
read=lambda name:list(csv.DictReader((SOURCE/name).open()))
instances=read('crop_instances.csv')
by_life=defaultdict(lambda:{'harvest':0,'fertilize':0,'water':0})
with (SOURCE/'unit_events.csv').open() as f:
    for e in csv.DictReader(f):
        if e['source'] not in ['WHEAT','CARROT','MELON','TOMATO','STRAWBERRY'] or not e['origin_day']:continue
        key=tuple(e[k] for k in ['episode_id','team','x','y','origin_day','source'])
        if e['operation']=='HARVEST':by_life[key]['harvest']+=int(e['quantity'])
        if e['operation']=='FERTILIZE':by_life[key]['fertilize']+=1
        if e['operation']=='WATER':by_life[key]['water']+=1
seats={(r['episode_id'],r['team']):int(r['seat']) for r in read('game_summary.csv')}
buckets=defaultdict(list)
tiles=defaultdict(list)
for r in instances:
    key=tuple(r[k] for k in ['episode_id','team','x','y','origin_day','name'])
    values=by_life[key]
    life={**r,**values,'seat':seats[r['episode_id'],r['team']]}
    tiles[tuple(r[k] for k in ['episode_id','team','x','y'])].append(life)
    if r['name'] in ['WHEAT','CARROT','MELON'] and r['outcome']=='harvest':
        buckets[r['name'],int(r['age_days']),values['harvest'],bool(values['fertilize'])].append(life)
summary=[]
for (item,age,quantity,fert),lives in buckets.items():
    summary.append({'item':item,'harvest_age':age,'quantity':quantity,'fertilized':fert,'count':len(lives),
        'teams':sorted({l['team'] for l in lives}),
        'mean_water_actions':statistics.mean(l['water'] for l in lives),
        'mean_fertilizer_actions':statistics.mean(l['fertilize'] for l in lives),
        'mean_elapsed_turns':statistics.mean(int(l['end_state'])-int(l['start_state']) for l in lives),
        'examples':[{k:l[k] for k in ['episode_id','team','seat','x','y','origin_day','start_state','end_state','harvest','water','fertilize']} for l in lives[:3]]})
summary.sort(key=lambda r:(r['item'],-r['count']))
motifs=defaultdict(list)
gaps=[]
for key,lives in tiles.items():
    lives.sort(key=lambda l:int(l['start_state']))
    for i,l in enumerate(lives):
        if i+1<len(lives):
            gap=int(lives[i+1]['start_state'])-int(l['end_state'])
            assert gap>=0
            gaps.append({'from':l['name'],'to':lives[i+1]['name'],'gap':gap,'team':l['team']})
        if l['name']!='MELON' or i+1==len(lives):continue
        suffix=lives[i+1:i+4]
        signature=tuple((n['name'],int(n['age_days']),n['harvest'],bool(n['fertilize'])) for n in suffix)
        motifs[signature].append({'episode_id':l['episode_id'],'team':l['team'],'seat':l['seat'],'x':l['x'],'y':l['y'],
            'calendar':[{k:n[k] for k in ['name','origin_day','start_state','end_state','harvest','fertilize','water']} for n in [l,*suffix]]})
motif_rows=[{'signature':s,'count':len(v),'teams':sorted({a['team'] for a in v}),'examples':v[:3]} for s,v in motifs.items()]
motif_rows.sort(key=lambda r:-r['count'])
transitions=[]
for a,b in sorted({(g['from'],g['to']) for g in gaps}):
    values=[g['gap'] for g in gaps if g['from']==a and g['to']==b]
    transitions.append({'from':a,'to':b,'count':len(values),'median_gap_turns':statistics.median(values),
        'within_one_turn':sum(v<=1 for v in values),'within_one_day':sum(v<=24 for v in values)})
report={'source':str(SOURCE.relative_to(EXP)),'cohort_player_games':72,'one_shot_buckets':summary,'post_melon_motifs':motif_rows,'transitions':transitions,
    'source_sha256':{n:hashlib.sha256((SOURCE/n).read_bytes()).hexdigest() for n in ['crop_instances.csv','unit_events.csv','game_summary.csv']},
    'limits':['Observed calendars and output are descriptive; they do not establish optimal service, profitability, or independent donor lineages.',
        'Harvested one-shot cohorts only in the age/yield table; ongoing crop motifs can end by DIG or decay.',
        'Days and coordinates retain raw zero-based replay conventions. Specific episode/seat/tile examples preserve provenance.']}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Crop renewal learned from recent top-player replays\n\nSource:07:08 top72player-games. All operations come from the replay analyzer\'s checked successful effects. Counts can include shared strategy families; frequency alone does not establish optimality.\n\n| Crop | Harvest age | Harvested quantity | Fertilized | Instances | Teams | Mean water actions |\n| --- | ---: | ---: | --- | ---: | ---: | ---: |\n'
for item in ['WHEAT','CARROT','MELON']:
    for r in [r for r in summary if r['item']==item][:6]:
        text+=f"| {item} | {r['harvest_age']} | {r['quantity']} | {r['fertilized']} | {r['count']} | {len(r['teams'])} | {r['mean_water_actions']:.2f} |\n"
text+='\nMost common post-melon crop calendars (name, harvest/end age, total harvested, any fertilizer):\n'
for r in motif_rows[:8]:text+=f"- {r['count']} tiles across {len(r['teams'])} teams: {r['signature']}.\n"
text+='\nANALYSIS.json stores exact episode/seat/tile calendars for each bucket and motif, plus transition gaps. Use them as proposals, then account for input cost, future prices and worker feasibility.\n'
(RUN/'README.md').write_text(text)
print(text)
