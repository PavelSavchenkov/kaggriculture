from pathlib import Path
import json
import subprocess
import numpy as np

RUN=Path(__file__).resolve().parent
games=[]
for opponent in ['empty_sale_slots_m2','public_router']:
    games+=json.loads((RUN/'discovery'/f'cold_renewal_p0_vs_{opponent}.json').read_text())['games']
assert len(games)==32
with (RUN/'paired_scenarios.txt').open('w') as f:
    f.write(f'two_sided_v1 {len(games)}\n')
    for g in games:
        p=g['opponent_profile'];fixed=sum(r[1] for r in p['fixed_costs'])
        f.write(f"{g['seed']} {g['seat']} {g['cash']} 0 0 {len(p['flows'])} {fixed}\n")
        f.write(' '.join(map(str,g['shops']))+'\n')
        for row in p['flows']:f.write(' '.join(map(str,row))+'\n')
command=['conda','run','-n','kaggriculture',str(RUN/'search'),str(RUN/'paired_scenarios.txt'),str(RUN/'PAIRED_ESTIMATES.json')]
subprocess.run(command,check=True)
estimated={f"cold_renewal_p{p['id']}":p for p in json.loads((RUN/'PAIRED_ESTIMATES.json').read_text())['proposals']}
actual=json.loads((RUN/'ANALYSIS.json').read_text())['summary']
control=estimated['cold_renewal_p0']
rows=[]
for a in actual:
    e=estimated[a['agent']]
    rows.append({'agent':a['agent'],'predicted_cash_gain':e['cash']-control['cash'],'actual_cash_gain':a['cash_gain'],
        'predicted_margin_gain':e['margin']-control['margin'],'actual_margin_gain':a['margin_gain'],
        'predicted_output':e['produced'],'actual_output':a['actual_produced']})


def ranks(values):
    a=np.asarray(values)
    return np.array([np.sum(a<v)+.5*(np.sum(a==v)-1) for v in a])


correlations={}
for key in ['cash','margin']:
    p=[r[f'predicted_{key}_gain'] for r in rows];a=[r[f'actual_{key}_gain'] for r in rows]
    correlations[key]={'pearson':float(np.corrcoef(p,a)[0,1]),'spearman':float(np.corrcoef(ranks(p),ranks(a))[0,1]),
        'gain_mae':float(np.mean(np.abs(np.array(p)-a)))}
report={'games':32,'candidates':len(rows),'command':command,'rows':rows,'correlations':correlations,
    'scope':'Matched discovery seeds and original-compiler opponent controls. Every proposal is priced against those fixed rival flows. Actual candidate games can change rival behavior; forecast deviations therefore include opponent response, own execution, service and timing assumptions.',
    'limits':'These11 candidates were selected using an earlier64-scenario panel. Correlations on this selected exposed subset are diagnostic, not held-out ranking validation.'}
(RUN/'CALIBRATION.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Matched-scenario estimator diagnosis\n\n'+report['scope']+'\n\n'+report['limits']+'\n\n| Candidate | Predicted cash gain | Actual cash gain | Predicted margin gain | Actual margin gain |\n| --- | ---: | ---: | ---: | ---: |\n'
for r in rows:text+=f"| {r['agent']} | {r['predicted_cash_gain']:+.2f} | {r['actual_cash_gain']:+.2f} | {r['predicted_margin_gain']:+.2f} | {r['actual_margin_gain']:+.2f} |\n"
text+='\nCorrelations and gain MAE: '+json.dumps(correlations)+'\n'
(RUN/'CALIBRATION.md').write_text(text)
print(text)
