from datetime import datetime, timezone
from pathlib import Path
from statistics import mean
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
BASE='rival_wool_context_v3'
variants=list(json.loads((RUN/'LINEAGE.json').read_text())['variants'])
rows={a:{} for a in variants}
for a in variants:
    for p in sorted((RUN/'discovery').glob(f'{a}_vs_*.json')):
        data=json.loads(p.read_text());b=data['agent_b'];games=data['games']
        control=json.loads((EXP/f'runs/observed_sale_lead_003/discovery/{BASE}_vs_{b}.json').read_text())['games'][:64]
        assert len(games)==64 and [(g['seed'],g['seat']) for g in games]==[(g['seed'],g['seat']) for g in control]
        margin=[g['cash']-g['opponent_cash'] for g in games];old=[g['cash']-g['opponent_cash'] for g in control]
        gain=[x-y for x,y in zip(margin,old)]
        row={'games':64,'wins':sum(x>0 for x in margin),'ties':sum(x==0 for x in margin),
             'utility_gain':mean((x>0)+.5*(x==0)-(y>0)-.5*(y==0) for x,y in zip(margin,old)),
             'mean_margin_gain':mean(gain),'paired_margin_min':min(gain),'paired_margin_worse':sum(x<0 for x in gain)}
        rows[a][b]=row
    assert len(rows[a])==10
eligible=[a for a,values in rows.items() if all(v['utility_gain']>=0 and v['mean_margin_gain']>=0 for v in values.values()) and values[BASE]['paired_margin_min']>=0]
summary={a:{'utility_gain':mean(x['utility_gain'] for x in rows[a].values()),'mean_margin_gain':mean(x['mean_margin_gain'] for x in rows[a].values()),'eligible':a in eligible} for a in variants}
assert eligible,'No globally delayed variant meets the discovery screen.'
selected=max(eligible,key=lambda a:(summary[a]['utility_gain'],summary[a]['mean_margin_gain']))
record={'created_utc':datetime.now(timezone.utc).isoformat(),'scope':'Four global start times, ten opponents,64 games each; exposed discovery, controls are exact first64 old256 records.','comparisons':rows,'summary':summary,'selected':selected,'selection':'Among variants with no individual utility/mean regression and no paired parent margin loss, maximize equal-opponent utility, then mean margin. Full256 extension and all fresh/operational audits still required.'}
(RUN/'DISCOVERY_ANALYSIS.json').write_text(json.dumps(record,indent=2)+'\n')
print('Selected',selected,summary)
