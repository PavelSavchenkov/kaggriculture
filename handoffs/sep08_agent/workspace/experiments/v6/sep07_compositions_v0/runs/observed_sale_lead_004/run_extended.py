from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from statistics import mean
import json
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
analysis=json.loads((RUN/'DISCOVERY_ANALYSIS.json').read_text());agent=analysis['selected']
binary=json.loads((RUN/'BUILD.json').read_text())['binary']
opponents=list(analysis['comparisons'][agent]);output=RUN/'extended';output.mkdir(exist_ok=False)


def run(b):
    p=output/f'{agent}_vs_{b}.json'
    cmd=['conda','run','-n','kaggriculture',binary,'--a',agent,'--b',b,'--games','128','--seed-start','1000','--seat-mode','both','--threads','6','--profile','--validate','--output',str(p)]
    with p.with_suffix('.log').open('w') as f:subprocess.run(cmd,stdout=f,stderr=subprocess.STDOUT,check=True)
    data=json.loads(p.read_text());games=data['games'];control=json.loads((EXP/f'runs/observed_sale_lead_003/discovery/rival_wool_context_v3_vs_{b}.json').read_text())['games']
    assert len(games)==len(control)==256 and [(x['seed'],x['seat']) for x in games]==[(x['seed'],x['seat']) for x in control]
    margins=[x['cash']-x['opponent_cash'] for x in games];old=[x['cash']-x['opponent_cash'] for x in control];gains=[x-y for x,y in zip(margins,old)]
    row={'games':256,'wins':sum(x>0 for x in margins),'ties':sum(x==0 for x in margins),
         'utility_gain':mean((x>0)+.5*(x==0)-(y>0)-.5*(y==0) for x,y in zip(margins,old)),
         'mean_margin_gain':mean(gains),'paired_margin_min':min(gains),'paired_margin_worse':sum(x<0 for x in gains),
         'own_cash_gain':mean(x['cash']-y['cash'] for x,y in zip(games,control)),
         'rival_cash_gain':mean(x['opponent_cash']-y['opponent_cash'] for x,y in zip(games,control)),
         'production_equal_games':sum(x['produced']==y['produced'] for x,y in zip(games,control)),
         'fault_gain':mean(x['unit_faults']-y['unit_faults'] for x,y in zip(games,control)),
         'worker_days_gain':mean(x['worker_days']-y['worker_days'] for x,y in zip(games,control)),'command':cmd}
    print(b,row['wins'],row['utility_gain'],row['mean_margin_gain'],flush=True);return b,row


with ThreadPoolExecutor(max_workers=3) as pool:rows=dict(pool.map(run,opponents))
passed=all(x['utility_gain']>=0 and x['mean_margin_gain']>=0 for x in rows.values()) and rows['rival_wool_context_v3']['paired_margin_min']>=0
(RUN/'EXTENDED_ANALYSIS.json').write_text(json.dumps({'candidate':agent,'baseline':'rival_wool_context_v3','comparisons':rows,'screen_passed':passed,'scope':'Exposed1000..1127 full256 extension; no fresh claim.'},indent=2)+'\n')
print('Extended screen passed',passed)
