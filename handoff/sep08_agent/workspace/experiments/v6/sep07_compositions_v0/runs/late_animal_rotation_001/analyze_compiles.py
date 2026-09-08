from pathlib import Path
from datetime import datetime, timezone
import json
import statistics

RUN=Path(__file__).resolve().parent
report={'created_utc':datetime.now(timezone.utc).isoformat(),'scope':'64 discovery games per forced source leaf. Candidate versus source and unchanged-crop compiler control. This is not a fresh promotion panel.','leaves':{}}
for leaf in ['off','on']:
    source=json.loads((RUN/f'goose_c31_d13_{leaf}_002/exact/parent.json').read_text())['games']
    candidate=json.loads((RUN/f'goose_c31_d13_{leaf}_002/exact/candidate.json').read_text())['games']
    control=json.loads((RUN/f'control_c31_d13_{leaf}_002/exact/candidate.json').read_text())['games']
    assert len(source)==len(candidate)==len(control)==64
    for a,b,c in zip(source,candidate,control):assert (a['seed'],a['seat'])==(b['seed'],b['seat'])==(c['seed'],c['seat'])
    activated=[]
    for i,game in enumerate(candidate):
        added=[life for life in game['profile']['lives'] if life[0]==9 and life[1:3]==[1,3] and life[5]==13]
        if added:
            assert len(added)==1 and added[0][6].bit_count()==17
            activated.append(i)
    def contrast(a,b,indices):
        result={}
        for key in ['cash','opponent_cash','worker_days','unit_faults','revenue','spend']:
            result[key+'_gain']=statistics.mean(a[i][key]-b[i][key] for i in indices)
        result['margin_gain']=result['cash_gain']-result['opponent_cash_gain']
        for key in ['hires','hire_cost','weed_digs']:
            result[key+'_gain']=statistics.mean(a[i]['profile'][key]-b[i]['profile'][key] for i in indices)
        for key in ['produced','sold','discarded']:
            result[key+'_gain']=[statistics.mean(a[i][key][p]-b[i][key][p] for i in indices) for p in range(12)]
        return result
    value={'activated_games':len(activated),'vs_source':contrast(candidate,source,range(64)),
           'vs_control':contrast(candidate,control,range(64)),
           'control_vs_source':contrast(control,source,range(64)),
           'activated_vs_control':contrast(candidate,control,activated),
           'seed1000_vs_control':contrast(candidate,control,[0,1]),
           'unactivated_full_records_identical':sum(candidate[i]==source[i] for i in range(64) if i not in activated)}
    report['leaves'][leaf]=value
    print(leaf,'activated',len(activated))
    print('whole',value['vs_control'])
    print('activated',value['activated_vs_control'])
    print('control',value['control_vs_source'])
report['conclusion']='Do not promote the unconditional goose edit. It adds the predicted animal output and preserves later farm requirements but extra hires make this discovery cohort weaker. Use actual compiled labor costs in valuation, and test observed-shop contexts and other species. Source-control changes are small and reported separately.'
(RUN/'COMPOSITION_CAUSAL.json').write_text(json.dumps(report,indent=2)+'\n')
