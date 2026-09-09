from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,statistics,subprocess
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
names=json.loads((RUN/'LINEAGE.json').read_text())['variants']
opponents=['empty_sale_slots_m2','teammate_shoprouter','public_router_v52','ahmed_v23','junghoon_wool_sales','pass']
binary=json.loads((RUN/'BUILD.json').read_text())['binary']
commands=[]
for opponent in opponents:
 for name in names[:2]:
  actual=json.loads((RUN/'discovery'/f'{name}_vs_{opponent}.json').read_text())['games']
  expected=json.loads((EXP/'runs/titan_sale_agent_sep08_001/discovery'/f'{name}_vs_{opponent}.json').read_text())['games']
  assert actual==expected
rows=[]
for opponent in opponents:
 baseline=json.loads((RUN/'discovery'/f'{names[0]}_vs_{opponent}.json').read_text())['games']
 for name in names:
  d=json.loads((RUN/'discovery'/f'{name}_vs_{opponent}.json').read_text());games=d['games']
  assert len(games)==32 and all(g['turns']==719 for g in games)
  mean=lambda fn:statistics.mean(fn(g) for g in games)
  gain=lambda fn:statistics.mean(fn(a)-fn(b) for a,b in zip(games,baseline))
  row={'agent':name,'opponent':opponent,'games':32,'wins':sum(g['cash']>g['opponent_cash'] for g in games),'ties':sum(g['cash']==g['opponent_cash'] for g in games),'cash':mean(lambda g:g['cash']),'margin':mean(lambda g:g['cash']-g['opponent_cash']),'cash_gain':gain(lambda g:g['cash']),'margin_gain':gain(lambda g:g['cash']-g['opponent_cash']),'fault_gain':gain(lambda g:g['unit_faults']),'hire_cost_gain':gain(lambda g:g['profile']['hire_cost']),'changed_games':sum(a!=b for a,b in zip(games,baseline))}
  for key in ['produced','sold','discarded']:row[key+'_gain']=[gain(lambda g,i=i,key=key:g[key][i]) for i in range(12)]
  row['physical_records_equal']=sum(all(a[k]==b[k] for k in ['produced','sold','discarded','unit_faults','worker_days']) and a['profile']['hires']==b['profile']['hires'] for a,b in zip(games,baseline))
  rows.append(row)
report={'completed_utc':datetime.now(timezone.utc).isoformat(),'games':960,'exact_control_games':384,'rows':rows,'control_commands':commands,'decision':'Retain the end-stock/deadline contract as a useful integration correction. Closed-loop forecast alone does not fix the failure. No promotion: endpoint-contract variants still have opponent regressions. Diagnose market-model errors separately from remaining production/guard changes.','operational':json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())['games']}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Sale-search integration discovery\n\n'+report['decision']+'\n\n960 discovery games, 384 exact old control records; operational evidence is recorded separately.\n\n| Agent | Opponent | W/T/L | Cash gain | Margin gain | Fault gain |\n|---|---|---:|---:|---:|---:|\n'
for row in rows:
 if row['agent']!=names[0]:text+=f"| {row['agent']} | {row['opponent']} | {row['wins']}/{row['ties']}/{32-row['wins']-row['ties']} | {row['cash_gain']:+.2f} | {row['margin_gain']:+.2f} | {row['fault_gain']:+.2f} |\n"
(RUN/'RESULTS.md').write_text(text)
paths=[p for p in RUN.rglob('*') if p.is_file() and p.suffix in {'.hpp','.cpp'}]
(RUN/'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},indent=2)+'\n')
print('960 discovery games and384 exact old control games analyzed.',flush=True)
