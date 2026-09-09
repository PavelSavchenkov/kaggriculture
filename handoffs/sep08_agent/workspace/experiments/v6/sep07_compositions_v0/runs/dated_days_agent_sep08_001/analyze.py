from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import statistics

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
spec=json.loads((RUN/'LINEAGE.json').read_text())
rows=[];control_games=0
for opponent in spec['discovery']['opponents']:
    parent=json.loads((RUN/f'discovery/dated_expansion_p362_vs_{opponent}.json').read_text())
    control=json.loads((RUN/f'discovery/dated_days_control_vs_{opponent}.json').read_text())
    assert parent['games']==control['games'];control_games+=len(parent['games'])
    for mode in ['control','labor','feed','combined']:
        name=f'dated_days_{mode}';data=json.loads((RUN/f'discovery/{name}_vs_{opponent}.json').read_text())
        pairs=list(zip(data['games'],parent['games']))
        mean=lambda f:statistics.mean(f(a,b) for a,b in pairs)
        rows.append({'name':name,'opponent':opponent,'games':len(pairs),'utility':data['win_utility'],
            'cash_gain':mean(lambda a,b:a['cash']-b['cash']),
            'margin_gain':mean(lambda a,b:(a['cash']-a['opponent_cash'])-(b['cash']-b['opponent_cash'])),
            'hire_cost_gain':mean(lambda a,b:a['profile']['hire_cost']-b['profile']['hire_cost']),
            'produced_gain':[mean(lambda a,b,i=i:a['produced'][i]-b['produced'][i]) for i in range(9)],
            'changed_records':sum(a!=b for a,b in pairs),
            'changed_own_actions':sum(a['action_hash']!=b['action_hash'] for a,b in pairs)})
coverage=[];exact=0
for path in sorted((RUN/'coverage_results').glob('*.json')):
    data=json.loads(path.read_text())
    original=json.loads((RUN/'discovery'/path.name).read_text())
    assert data['games']==original['games'];exact+=len(data['games'])
    counts=[json.loads(line) for line in path.with_suffix('.coverage.jsonl').read_text().splitlines()]
    assert all(r['hours']==24*r['days'].bit_count() and r['unit_mismatches']==0 for r in counts)
    coverage.append({'pair':path.stem,'games':len(counts),'activated_games':sum(bool(r['days']) for r in counts),
        'days':sum(r['days'].bit_count() for r in counts),'hours':sum(r['hours'] for r in counts),
        'day_masks':sorted(set(r['days'] for r in counts)),'unit_mismatches':0})
ops=json.loads((RUN/'OPERATIONAL_CHECKS.json').read_text())
report={'created_utc':datetime.now(timezone.utc).isoformat(),'discovery_games':320,'exact_empty_wrapper_records':control_games,
    'coverage_full_records_equal':exact,'coverage':coverage,'operational_games':ops['games'],'rows':rows,
    'decision':'Useful exact day schedules transfer only to matching public-router states in this small panel. All other tested opponent records retain the parent. No parent or league promotion.',
    'next':'Fix general input delivery in the reactive compiler. The day solver proves missed feeding and worker savings are jointly feasible, but one exact template per source state is too narrow to realize the full objective.',
    'limits':spec['limits']}
(RUN/'ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
text='# Guarded day integration results\n\n'+report['decision']+'\n\n'
text+=f"320 discovery games;{control_games} exact empty-wrapper controls;{exact} exact coverage games;{ops['games']} operational games. Every selected day executes all24 planned hours with the expected worker count.\n\n"
text+='| Policy | Opponent | Cash gain | Margin gain | Hire cost change | Changed games |\n| --- | --- | ---: | ---: | ---: | ---: |\n'
for r in rows:
    text+=f"| {r['name']} | {r['opponent']} | {r['cash_gain']:+.3f} | {r['margin_gain']:+.3f} | {r['hire_cost_gain']:+.3f} | {r['changed_records']} |\n"
text+='\n'+report['next']+'\n';(RUN/'RESULTS.md').write_text(text)
(RUN/'README.md').write_text('# Complete cold-farm day schedule integration\n\nRead RESULTS.md and ANALYSIS.json. Four C++ policies separate unchanged control, labor-only schedules, added feeding with original hires, and combined schedules. Entry uses exact observed own farm/stock/seed state. Coverage confirms every activated day executes all24hours; activation is narrow. These policies do not improve the strong incumbent. Source plans and exact day checks are in dated_day_service_sep08_001/002.\n')
key=str((RUN/'prepare.py').relative_to(EXP));before=spec['source_hashes'][key];after=hashlib.sha256((RUN/'prepare.py').read_bytes()).hexdigest()
(RUN/'BUILD_AMENDMENT.json').write_text(json.dumps({'generator_before':before,'generator_after':after,
    'reason':'Initial build rejected the bare alias-template base constructor. Added explicit base template arguments in generator and four agent headers before the successful build. Failed build log preserved. No successful earlier policy build was replaced.'},indent=2)+'\n')
print('320 discovery,64 wrapper controls,128 exact coverage,48 operational games.')
for r in rows:
    if r['opponent']=='public_router':print(r)
print('Coverage',coverage)
