"""Compare exact saved outcomes and forecast rankings, never choose gameplay."""
import csv
import json
import statistics
from collections import Counter
from pathlib import Path
from generate import NAMES
from screen import RIVALS

RUN=Path(__file__).resolve().parent


def main():
    panels={};metrics=[];decisions=[];parity=[]
    for name in NAMES:
        for rival in RIVALS:
            result=json.loads((RUN/f'results/discovery_{name}_vs_{rival}.json').read_text());panels[name,rival]={(g['seed'],g['seat']):g for g in result['games']}
            games=result['games'];metrics.append({'agent':name,'opponent':rival,'games':len(games),'wins':sum(g['cash']>g['opponent_cash']for g in games),'ties':sum(g['cash']==g['opponent_cash']for g in games),'mean_cash':statistics.mean(g['cash']for g in games),'mean_margin':statistics.mean(g['cash']-g['opponent_cash']for g in games),'mean_faults':statistics.mean(g['unit_faults']for g in games),'mean_discarded':statistics.mean(sum(g['discarded'])for g in games)})
    for name in NAMES[2:]:
        for rival in RIVALS:
            for trace in json.loads((RUN/f'results/trace_{name}_vs_{rival}.json').read_text()):
                key=trace['seed'],trace['seat'];actual=panels[name,rival][key];controls=[panels[n,rival][key]for n in NAMES[:2]];selected=trace['branch'];control=controls[selected]
                assert actual==control,(name,rival,key,selected)
                margins=[g['cash']-g['opponent_cash']for g in controls];scores=[x['own']-x['rival']for x in trace['estimates']]
                decisions.append({'agent':name,'opponent':rival,'seed':key[0],'seat':key[1],'branch':selected,'margin':margins[selected],'control_margins':margins,'regret':max(margins)-margins[selected],'forecast_margin_delta':scores[1]-scores[0],'actual_margin_delta':margins[1]-margins[0],'estimated':trace['estimates'],'own_cash_before288':trace['cash_before288'],'own_wheat_before288':trace['shed_wheat_before288']})
    for rival in RIVALS:
        traces=[json.loads((RUN/f'results/trace_{name}_vs_{rival}.json').read_text())for name in NAMES[:2]]
        for a,b in zip(*traces):
            assert (a['seed'],a['seat'])==(b['seed'],b['seat'])
            assert a['physical_before288']==b['physical_before288'] and a['cash_before288']==b['cash_before288'] and a['rival_cash_before288']==b['rival_cash_before288']
            parity.append((rival,a['seed'],a['seat']))
    aggregate=[]
    for name in NAMES:
        rows=[x for x in metrics if x['agent']==name];d=[x for x in decisions if x['agent']==name]
        aggregate.append({'agent':name,'games':sum(x['games']for x in rows),'wins':sum(x['wins']for x in rows),'ties':sum(x['ties']for x in rows),'mean_margin':statistics.mean(x['mean_margin']for x in rows),'mean_faults':statistics.mean(x['mean_faults']for x in rows),**({'regret':statistics.mean(x['regret']for x in d),'best_choice':sum(x['regret']==0 for x in d),'branch_counts':dict(Counter(x['branch']for x in d))}if d else{})})
    report={'scope':'Discovery seeds1000..1031 bothseats, five opponents. No fresh promotion audit yet.','metrics':aggregate,'checks':{'adaptive_games_equal_selected_fixed_course':len(decisions),'both_fixed_prefix288_physical_cash_equal_contexts':len(parity)},'limitations':['Branch controls can have invalid unit attempts or incomplete donor realization; inspect profiles','Different market objectives and source quantities need separate forecast error attribution','No donor private formula or profitable waiting claim']}
    for name,data in [('REPORT',report),('decision_cases',decisions)]: (RUN/f'{name}.json').write_text(json.dumps(data,indent=2)+'\n')
    with (RUN/'metrics.csv').open('w')as f:writer=csv.DictWriter(f,fieldnames=metrics[0]);writer.writeheader();writer.writerows(metrics)
    print(json.dumps(report,indent=2),flush=True)


if __name__=='__main__':main()
