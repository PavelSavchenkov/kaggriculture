"""Attribute complete-course transfer using saved exact C++ outcomes."""
import json
import statistics as s
from collections import Counter,defaultdict
from pathlib import Path
from screen import RIVALS

RUN=Path(__file__).resolve().parent
NAMES=['sheep_fixed_small','sheep_fixed_expansion']


def meanrows(rows):return [s.mean(row[i]for row in rows)for i in range(len(rows[0]))]


def main():
    report={'scope':'Original replay exact C++ profile compared with80independent-shop local games per fixed branch (seeds1000..1007both, five opponents). Donor versus other environments is descriptive, not a causal intervention.','profiles':[],'shop_groups':[],'terminal':[],'operational':{}}
    for branch,label in enumerate(['small','expansion']):
        donor=json.loads((RUN/f'donor_replay/{label}_result.json').read_text());games=[g for rival in RIVALS for g in json.loads((RUN/f'results/profile_{NAMES[branch]}_vs_{rival}.json').read_text())['games']]
        p=donor['profile'];daily=lambda rows:[[sum(q[3]for q in rows if q[0]//24==day and q[1]==item)for item in range(9)]for day in range(30)]
        original=daily(p['flows']);fixed_original=sum(q[1]for q in p['fixed_costs']);models=json.loads((RUN/'flow_models.json').read_text())[branch]
        assert models['sales'][12:]==original[12:]
        assert sum(models['fixed_costs'][12:])==sum(q[1]for q in p['fixed_costs']if q[0]>=288)
        totals=[]
        for g in games:
            gp=g['profile'];actual=daily(gp['flows']);row={'seed':g['seed'],'seat':g['seat'],'hires':gp['hires'],'hire_cost':gp['hire_cost'],'fixed_cost_delta':sum(q[1]for q in gp['fixed_costs'])-fixed_original,'future_daily_sales_absolute_error':sum(abs(actual[d][i]-original[d][i])for d in range(12,30)for i in range(9)),'sheep_placed':sum(l[0]==11 for l in gp['lives']),'cows_placed':sum(l[0]==10 for l in gp['lives'])};totals.append(row)
        report['profiles'].append({'branch':label,'games':len(games),'donor_produced':donor['produced'],'local_mean_produced':meanrows([g['produced']for g in games]),'donor_sold':donor['sold'],'local_mean_sold':meanrows([g['sold']for g in games]),'donor_discarded':donor['discarded'],'local_mean_discarded':meanrows([g['discarded']for g in games]),'donor_successful':p['successful'],'local_mean_successful':meanrows([g['profile']['successful']for g in games]),'donor_hires':p['hires'],'local_hires':dict(Counter(t['hires']for t in totals)),'donor_hire_cost':p['hire_cost'],'local_hire_cost':dict(Counter(t['hire_cost']for t in totals)),'fixed_cost_delta_range':[min(t['fixed_cost_delta']for t in totals),max(t['fixed_cost_delta']for t in totals)],'future_daily_sales_mae_mean':s.mean(t['future_daily_sales_absolute_error']for t in totals),'sheep_placed':dict(Counter(t['sheep_placed']for t in totals)),'cows_placed':dict(Counter(t['cows_placed']for t in totals)),'source_flow_fixed_cost_model_exact':True})
    groups=defaultdict(list)
    for rival in RIVALS:
        panels=[json.loads((RUN/f'results/discovery_{n}_vs_{rival}.json').read_text())['games']for n in NAMES]
        for a,b in zip(*panels):
            assert (a['seed'],a['seat'])==(b['seed'],b['seat'])
            groups[sum(x==7 for x in a['shops'][:4])].append({'seed':a['seed'],'seat':a['seat'],'opponent':rival,'known_yarn':sum(x==7 for x in a['shops'][:4]),'all_yarn':sum(x==7 for x in a['shops']),'own_delta':b['cash']-a['cash'],'rival_delta':b['opponent_cash']-a['opponent_cash'],'margin_delta':b['cash']-b['opponent_cash']-a['cash']+a['opponent_cash']})
    for yarn,rows in sorted(groups.items()):report['shop_groups'].append({'known_yarn_at288':yarn,'contexts':len(rows),'expansion_better':sum(r['margin_delta']>0 for r in rows),'mean_margin_delta':s.mean(r['margin_delta']for r in rows),'mean_own_delta':s.mean(r['own_delta']for r in rows),'mean_rival_delta':s.mean(r['rival_delta']for r in rows)})
    for original in NAMES+['sheep_yarn2']:
        rows=[]
        for rival in RIVALS:
            a=json.loads((RUN/f'results/discovery_{original}_vs_{rival}.json').read_text())['games'];b=json.loads((RUN/f'results/discovery_{original}_terminal_vs_{rival}.json').read_text())['games']
            for x,y in zip(a,b):rows.append({'oldwin':x['cash']>x['opponent_cash'],'win':y['cash']>y['opponent_cash'],'margin_delta':y['cash']-y['opponent_cash']-x['cash']+x['opponent_cash'],'own_delta':y['cash']-x['cash']})
        report['terminal'].append({'agent':original+'_terminal','games':len(rows),'wins':sum(r['win']for r in rows),'win_delta':sum(r['win']-r['oldwin']for r in rows),'mean_margin_delta':s.mean(r['margin_delta']for r in rows),'mean_own_delta':s.mean(r['own_delta']for r in rows)})
    baseline=json.loads((RUN/'results/operational_16.json').read_text())['games']
    for mode in ['thread','masked','debug']:assert json.loads((RUN/f'results/operational_{mode}16.json').read_text())['games']==baseline
    report['operational']['generic_thread_masked_typeddebug_full_record_equal']=16
    for n in NAMES+['sheep_yarn2','sheep_value_margin_s64']:
        p=json.loads((RUN/f'results/{n}_pass128.json').read_text());q=json.loads((RUN/f'results/{n}_self16.json').read_text());report['operational'][n]={'pass_games':len(p['games']),'pass_J':p['pass_J'],'pass_mean_cash':p['mean_cash'],'self_games':len(q['games']),'all_turns719':all(g['turns']==719 for g in p['games']+q['games'])}
    (RUN/'TRANSFER_DIAGNOSTICS.json').write_text(json.dumps(report,indent=2)+'\n');(RUN/'shop_contexts.json').write_text(json.dumps([r for rows in groups.values()for r in rows],indent=2)+'\n');print(json.dumps(report,indent=2))


if __name__=='__main__':main()
