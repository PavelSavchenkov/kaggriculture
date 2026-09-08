from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,statistics
import numpy as np
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
protocol=json.loads((RUN/'FRESH_PROTOCOL.json').read_text())
coverage=json.loads((RUN/'COVERAGE.json').read_text())
operations=json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())
assert coverage['full_game_records_equal']==640 and operations['games']==64
inputs=json.loads((RUN/'FRESH_INPUTS.json').read_text())
for path,digest in inputs.items():assert hashlib.sha256((ROOT/path).read_bytes()).hexdigest()==digest,path
baseline,candidate=protocol['actors'];rows=[];seed_gains=[]
for opponent in protocol['opponents']:
 data={a:json.loads((RUN/'fresh'/f'{a}_vs_{opponent}.json').read_text())['games'] for a in protocol['actors']}
 expected=[(s,t) for s in range(2300000,2300128) for t in range(2)]
 assert all([(g['seed'],g['seat']) for g in games]==expected and all(g['turns']==719 for g in games) for games in data.values())
 a,b=data[candidate],data[baseline]
 gains=np.array([x['cash']-y['cash'] for x,y in zip(a,b)],dtype=float).reshape(128,2).mean(axis=1);seed_gains.append(gains)
 mean=lambda fn:statistics.mean(fn(g) for g in a)
 delta=lambda fn:statistics.mean(fn(x)-fn(y) for x,y in zip(a,b))
 row={'opponent':opponent,'games_per_actor':256,'wins':sum(g['cash']>g['opponent_cash'] for g in a),'ties':sum(g['cash']==g['opponent_cash'] for g in a),'utility':mean(lambda g:(g['cash']>g['opponent_cash'])+.5*(g['cash']==g['opponent_cash'])),'cash':mean(lambda g:g['cash']),'margin':mean(lambda g:g['cash']-g['opponent_cash']),'cash_gain':float(gains.mean()),'margin_gain':delta(lambda g:g['cash']-g['opponent_cash']),'utility_gain':delta(lambda g:(g['cash']>g['opponent_cash'])+.5*(g['cash']==g['opponent_cash'])),'fault_gain':delta(lambda g:g['unit_faults']),'hire_cost_gain':delta(lambda g:g['profile']['hire_cost']),'hires_gain':delta(lambda g:g['profile']['hires'])}
 for key in ['produced','sold','discarded']:row[key+'_gain']=[delta(lambda g,i=i,key=key:g[key][i]) for i in range(12)]
 for key,fn in [('cash',lambda g:g['cash']),('margin',lambda g:g['cash']-g['opponent_cash'])]:
  tail=lambda games:statistics.mean(sorted(fn(g) for g in games)[:26])
  row['cvar10_'+key+'_gain']=tail(a)-tail(b)
 rows.append(row)
per_seed=np.array(seed_gains).mean(axis=0)
rng=np.random.default_rng(8716);bootstrap=per_seed[rng.integers(0,128,size=(10000,128))].mean(axis=1);interval=np.quantile(bootstrap,[.025,.975]).tolist()
direct=next(r for r in rows if r['opponent']=='day_program_p362_m3')
gates={'all_opponent_cash_means_nonnegative':all(r['cash_gain']>=0 for r in rows),'pooled_cash_gain_lower95_positive':interval[0]>0,'direct_parent_positive_margin_and_utility':direct['margin']>0 and direct['utility']>=.5,'no_mean_fault_regression':all(r['fault_gain']<=0 for r in rows),'operations_and_coverage':True,'all_original_inputs_unchanged':True}
report={'completed_utc':datetime.now(timezone.utc).isoformat(),'candidate':candidate,'parent':'day_program_p362_m3','games':2560,'gates':gates,'mean_cash_gain':float(per_seed.mean()),'cash_gain95':interval,'rows':rows,'coverage_games':640,'operational_games':64,'input_files_unchanged':len(inputs),'decision':'Retain as the improved p362 cold-farm continuation.' if all(gates.values()) else 'Do not select as the p362 continuation; investigate failed fresh gates.','scope':'This improves one cold-farm compiler branch only. Global strongest reference remains empty_sale_slots_m2; no official catalog copy or submission. No claim that arbitrary composition compilation is complete.'}
(RUN/'FRESH_ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Fresh p362 service-bank confirmation\n\n'+report['decision']+' '+report['scope']+'\n\n2,560 fresh games; all declared gates '+('pass' if all(gates.values()) else 'do not pass')+f'. Equal-opponent mean cash gain {report["mean_cash_gain"]:+.2f}, 95% seed-cluster interval [{interval[0]:+.2f}, {interval[1]:+.2f}].\n\n| Opponent | W/T/L | Cash gain | Margin gain | Hire-cost gain |\n|---|---:|---:|---:|---:|\n'
for r in rows:text+=f"| {r['opponent']} | {r['wins']}/{r['ties']}/{256-r['wins']-r['ties']} | {r['cash_gain']:+.2f} | {r['margin_gain']:+.2f} | {r['hire_cost_gain']:+.2f} |\n"
(RUN/'FRESH_RESULTS.md').write_text(text)
print(json.dumps({k:report[k] for k in ['gates','mean_cash_gain','cash_gain95','decision']},indent=2),flush=True)
