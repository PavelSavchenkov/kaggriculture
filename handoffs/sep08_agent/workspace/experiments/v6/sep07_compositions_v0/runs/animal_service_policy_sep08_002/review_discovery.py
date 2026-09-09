"""Explain every guard miss and retain the complete paired discovery limits."""
from pathlib import Path
from collections import Counter, defaultdict
import hashlib
import json

RUN = Path(__file__).resolve().parent
OUT = RUN/'discovery_2390000'
protocol = json.loads((OUT/'PROTOCOL.json').read_text())
results = json.loads((RUN/'RESULTS.json').read_text())
audit = json.loads((RUN/'guard_audit_results/AUDIT.json').read_text())
traces = {r['name']:r for r in audit['results']}
allowed = {('tile','38',str(i)) for i in (0,1,3,4,5)} | {('seed','0','0'),('shed','0','0')}
count = Counter()
changed_values = []
parent = protocol['agents'][0]
for job in protocol['jobs']:
    name = ('native_' if job['native'] else '')+f"{job['a']}_vs_{job['b']}"
    diagnostics = json.loads((OUT/(name+'.json.diagnostics.json')).read_text())['games']
    missed = [g for g in diagnostics if g['missed_days']]
    if not missed:
        assert name not in traces
        continue
    trace = traces[name]
    assert trace['exact_full_records_equal']
    fields = defaultdict(list)
    for f in trace['differences']:
        fields[int(f['seed']),int(f['seat'])].append(f)
    assert set(fields)=={(g['seed'],g['seat']) for g in missed}
    for g in missed:
        rows = fields[g['seed'],g['seat']]
        days = {int(f['day']) for f in rows}
        assert sum(1<<d for d in days)==g['missed_days']
        first = [f for f in rows if int(f['day'])==min(days)]
        assert len(first)==1
        assert all(first[0][k]==v for k,v in {'day':'23','kind':'tile','item':'38','field':'0','expected':'0','actual':'2'}.items())
        assert all((f['kind'],f['item'],f['field']) in allowed for f in rows)
        count[job['a']]+=1
    if job['a']!=parent:
        before = name.replace(job['a'],parent,1)
        prior = traces[before]['differences']
        assert len(trace['differences'])==len(prior)
        for a,b in zip(trace['differences'],prior):
            if a==b:
                continue
            assert {k:v for k,v in a.items() if k!='actual'}=={k:v for k,v in b.items() if k!='actual'}
            assert all(a[k]==v for k,v in {'day':'29','family':'0','choice':'2','leaf':'0','kind':'shed','item':'0','field':'0','expected':'58','actual':'57'}.items())
            assert b['actual']=='55'
            changed_values.append({'matchup':name,'candidate':a,'parent':b})

assert sum(count.values())==len(results['missed_guards'])==42
comparisons = [r for r in results['rows'] if r['parent']==parent]
report = {'games':protocol['games'],'trace_games':audit['games'],'trace_matchups':len(traces),
    'missed_games_by_agent':dict(count),'guard_field_patterns_identical_to_parent':True,
    'changed_guard_values':changed_values,
    'guard_diagnosis':'Every miss begins with only a weed on tile38 at day23, then affects wheat tile/stock/seed fields. All checked animal states match. Some changed-course day29 entries have57 wheat rather than parent55 against expected58: the existing stock deficit is smaller. The first review incorrectly required identical values; field-by-field inspection corrected that assumption. No new failed obligation class appears.',
    'investigation_complete':True,'new_guard_patterns':0,
    'per_opponent_win_regressions':[r for r in comparisons if r['gain_win_pp']<0],
    'per_opponent_margin_regressions':[r for r in comparisons if r['gain_margin']<0],
    'per_opponent_cash_tail_regressions':[r for r in comparisons if r['gain_cash_tail']<0],
    'per_opponent_margin_tail_regressions':[r for r in comparisons if r['gain_margin_tail']<0],
    'groups':results['groups'],
    'scope':'Both variants have identical discovery records, unchanged choices and mean production, unchanged primary win counts, and small positive margin intervals. Useful execution candidate; no promotion and no broad audit claimed.',
    'audit_sha256':hashlib.sha256((RUN/'guard_audit_results/AUDIT.json').read_bytes()).hexdigest()}
(RUN/'FINAL_DISCOVERY_REVIEW.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:v for k,v in report.items() if k!='groups'},indent=2))
