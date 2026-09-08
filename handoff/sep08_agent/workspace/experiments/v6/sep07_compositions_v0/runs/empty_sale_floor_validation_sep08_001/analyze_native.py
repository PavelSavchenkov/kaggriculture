from datetime import datetime, timezone
from pathlib import Path
import json
import math
import statistics

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
SPEC=json.loads((RUN/'FRESH_PREREGISTERED.json').read_text())
CANDIDATE,BASELINE=SPEC['candidate'],SPEC['baseline']


def metrics(games):
    margins=[g['cash']-g['opponent_cash'] for g in games];cash=sorted(g['cash'] for g in games);tail=math.ceil(len(games)/10)
    return {'games':len(games),'wins':sum(x>0 for x in margins),'ties':sum(x==0 for x in margins),'losses':sum(x<0 for x in margins),
            'utility':statistics.mean((x>0)+.5*(x==0) for x in margins),'mean_margin':statistics.mean(margins),'mean_cash':statistics.mean(cash),
            'cvar10_margin':statistics.mean(sorted(margins)[:tail]),'pass_J':.8*statistics.mean(cash)+.2*statistics.mean(cash[:tail])}


def contrast(a,b):
    assert [(g['seed'],g['seat']) for g in a]==[(g['seed'],g['seat']) for g in b]
    ma,mb=metrics(a),metrics(b)
    row={key+'_gain':ma[key]-mb[key] for key in ['utility','mean_margin','mean_cash','cvar10_margin','pass_J']}
    for key in ['produced','sold','discarded']:
        row[key+'_gain']=[statistics.mean(x[key][i]-y[key][i] for x,y in zip(a,b)) for i in range(12)]
    for key in ['unit_faults','worker_days','opponent_cash']:
        row[key+'_gain']=statistics.mean(x[key]-y[key] for x,y in zip(a,b))
    for key in ['hires','hire_cost','land_cost']:
        row[key+'_gain']=statistics.mean(x['profile'][key]-y['profile'][key] for x,y in zip(a,b))
    row['changed_own_actions']=sum(x['action_hash']!=y['action_hash'] for x,y in zip(a,b))
    row['changed_rival_actions']=sum(x['opponent_action_hash']!=y['opponent_action_hash'] for x,y in zip(a,b))
    row['changed_production']=sum(x['produced']!=y['produced'] for x,y in zip(a,b))
    row['minimum_paired_margin_gain']=min(x['cash']-x['opponent_cash']-y['cash']+y['opponent_cash'] for x,y in zip(a,b))
    return row


def main():
    plan=json.loads((RUN/'NATIVE_PLAN.json').read_text());rows={}
    for opponent in dict.fromkeys(job[1] for job in plan['jobs']):
        games={a:json.loads((RUN/f'native/{a}_vs_{opponent}.json').read_text())['games'] for a in [CANDIDATE,BASELINE]}
        assert all(len(g)==256 and all(x['turns']==719 for x in g) for g in games.values())
        rows[opponent]={'metrics':{a:metrics(g) for a,g in games.items()},'contrast':contrast(games[CANDIDATE],games[BASELINE])}
    limits=plan['gates']
    gates={'all_means_nonnegative':all(v['contrast']['mean_margin_gain']>=limits['minimum_mean_margin_gain'] for v in rows.values()),
           'all_utilities_nonnegative':all(v['contrast']['utility_gain']>=limits['minimum_individual_utility_gain'] for v in rows.values()),
           'all_parent_paired_margins_nonnegative':rows[BASELINE]['contrast']['minimum_paired_margin_gain']>=0,
           'pass_J_nonnegative':rows['pass']['contrast']['pass_J_gain']>=limits['minimum_pass_J_gain']}
    report={'completed_utc':datetime.now(timezone.utc).isoformat(),'candidate':CANDIDATE,'baseline':BASELINE,'games':len(rows)*512,'gates':gates,'comparisons':rows}
    (RUN/'NATIVE_ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
    print('Native gates',gates)
    for o,v in rows.items():print(o,v['metrics'][CANDIDATE]['wins'],v['contrast']['utility_gain'],v['contrast']['mean_margin_gain'])


if __name__=='__main__':main()
