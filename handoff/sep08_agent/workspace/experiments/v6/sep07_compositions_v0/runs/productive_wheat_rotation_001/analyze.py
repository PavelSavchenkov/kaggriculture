"""Score paired policies and attribute realized production, trades and labor."""
import argparse
import csv
import json
import statistics as s
from collections import Counter
from pathlib import Path
from format_packages import NAMES
from screen import RIVALS

RUN=Path(__file__).resolve().parent
FULL=sum(1<<day for day in range(13,29))


def main():
    parser=argparse.ArgumentParser();parser.add_argument('phase',choices=['discovery','fresh','native']);args=parser.parse_args()
    names=NAMES if args.phase=='discovery'else['wheat_one_fert','wheat_one_plain','wheat_three_fert']
    rows=[];cases=[];parity=[]
    for name in names:
        for rival in RIVALS:
            a=json.loads((RUN/f'results/{args.phase}_{name}_vs_{rival}.json').read_text())['games']
            b=json.loads((RUN/f'results/{args.phase}_crop_value_m2_t4_vs_{rival}.json').read_text())['games']
            trace_path=RUN/f'results/trace_{name}_vs_{rival}.json';traces=json.loads(trace_path.read_text())if args.phase=='discovery'and trace_path.exists()else[{}]*len(a)
            for x,y,t in zip(a,b,traces):
                assert (x['seed'],x['seat'],x['shops'])==(y['seed'],y['seat'],y['shops'])
                if t:assert (x['seed'],x['seat'],x['cash'],x['opponent_cash'])==(t['seed'],t['seat'],t['cash'],t['opponent_cash'])
                cases.append({'name':name,'rival':rival,'seed':x['seed'],'seat':x['seat'],'matched_days':t.get('matched_days'),
                    'berry_selected':t.get('berry_selected'),'cash_delta':x['cash']-y['cash'],
                    'rival_cash_delta':x['opponent_cash']-y['opponent_cash'],
                    'margin_delta':x['cash']-x['opponent_cash']-y['cash']+y['opponent_cash'],
                    'output_delta':[v-w for v,w in zip(x['produced'],y['produced'])],
                    'sold_delta':[v-w for v,w in zip(x['sold'],y['sold'])],
                    'discard_delta':[v-w for v,w in zip(x['discarded'],y['discarded'])],
                    'worker_days_delta':x['worker_days']-y['worker_days'],
                    'fault_delta':x['unit_faults']-y['unit_faults'],
                    **({'hires_delta':x['profile']['hires']-y['profile']['hires'],
                        'hire_cost_delta':x['profile']['hire_cost']-y['profile']['hire_cost'],
                        'buy_delta':[v-w for v,w in zip(x['profile']['buys'],y['profile']['buys'])],
                        'seed_buy_delta':[v-w for v,w in zip(x['profile']['seed_buys'],y['profile']['seed_buys'])]}if'profile'in x else{})})
            subset=cases[-len(a):];margin=[x['cash']-x['opponent_cash']for x in a];baseline=[x['cash']-x['opponent_cash']for x in b]
            utility=lambda values:s.mean((v>0)+.5*(v==0)for v in values)
            row={'name':name,'rival':rival,'games':len(a),'wins':sum(v>0 for v in margin),'ties':sum(v==0 for v in margin),
                 'baseline_wins':sum(v>0 for v in baseline),'baseline_ties':sum(v==0 for v in baseline),
                 'win_utility':utility(margin),'win_utility_delta':utility(margin)-utility(baseline),
                 'mean_margin':s.mean(margin),'mean_margin_delta':s.mean(c['margin_delta']for c in subset),
                 'own_cash_delta':s.mean(c['cash_delta']for c in subset),'rival_cash_delta':s.mean(c['rival_cash_delta']for c in subset),
                 'margin_cvar10_delta':s.mean(sorted(margin)[:max(1,len(a)//10)])-s.mean(sorted(baseline)[:max(1,len(a)//10)]),
                 'changed_actions':sum(x['action_hash']!=y['action_hash']for x,y in zip(a,b)),
                 'output_delta':[s.mean(c['output_delta'][i]for c in subset)for i in range(9)]}
            if traces[0]:row['guard_masks']=dict(Counter(c['matched_days']for c in subset));row['berry_choices']=sum(c['berry_selected']for c in subset)
            rows.append(row)
            if args.phase=='discovery'and rival=='public_router':
                source=name.removeprefix('wheat_').removesuffix('_unclosed')
                fixed={key:json.loads((RUN/(source+suffix)/'exact/candidate.json').read_text())['games']for key,suffix in [('off',''),('on','_berry')]}
                for i,(game,trace)in enumerate(zip(a,traces)):
                    if trace and trace['matched_days']==FULL:
                        expected=fixed['on'if trace['berry_selected']else'off'][i]
                        parity.append({'name':name,'seed':game['seed'],'seat':game['seat'],'berry':trace['berry_selected'],
                                       'exact_fixed_course_record':game==expected})
    report={'phase':args.phase,'metrics':rows,'fixed_course_parity':parity,'scope':'Paired complete C++ games. Current parent is crop_value_m2_t4; unclosed control deliberately drops optional berry continuation. Biological output and net cash are separate from trade quantities.'}
    if args.phase=='discovery':
        expected={'wheat_one_fert':[6,0,0,0,0,0,0,0,0], 'wheat_one_plain':[0]*9,
                  'wheat_three_fert':[15,3,0,0,0,0,0,0,0],'wheat_three_plain':[-3,3,0,0,0,0,0,0,0]}
        report['biology_deviations']=[c for c in cases if c['name']in expected and c['matched_days']==FULL and c['output_delta'][:9]!=expected[c['name']]]
        report['partial_guard_cases']=[c for c in cases if c['matched_days']not in [None,0,FULL]]
    (RUN/f'{args.phase.upper()}_REPORT.json').write_text(json.dumps(report,indent=2)+'\n')
    (RUN/f'{args.phase}_cases.json').write_text(json.dumps(cases,separators=(',',':'))+'\n')
    with (RUN/f'{args.phase}_metrics.csv').open('w')as f:w=csv.DictWriter(f,fieldnames=rows[0]);w.writeheader();w.writerows(rows)
    for name in names:
        subset=[r for r in rows if r['name']==name]
        print(name,'utility_delta',s.mean(r['win_utility_delta']for r in subset),'margin_delta',s.mean(r['mean_margin_delta']for r in subset),'rows',len(subset))


if __name__=='__main__':main()
