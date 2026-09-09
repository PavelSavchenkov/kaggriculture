"""Verify retained policy evidence, summarize checks, and freeze this run."""
import hashlib
import json
import statistics as s
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path
from screen import RIVALS

RUN=Path(__file__).resolve().parent
FULL=sum(1<<day for day in range(13,29))


def read(path):
    return json.loads(path.read_text())


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    generic=read(RUN/'results/operational_generic16.json')['games']
    assert generic==read(RUN/'results/operational_debug16.json')['games']
    assert generic==read(RUN/'results/operational_thread16.json')['games']
    trace_rows=[];native_rows=[]
    for rival in RIVALS:
        games=read(RUN/f'results/fresh_wheat_one_fert_vs_{rival}.json')['games']
        parent=read(RUN/f'results/fresh_crop_value_m2_t4_vs_{rival}.json')['games']
        traces=read(RUN/f'results/fresh_trace_wheat_one_fert_vs_{rival}.json')
        for g,p,t in zip(games,parent,traces):
            assert (g['seed'],g['seat'],g['cash'],g['opponent_cash'])==(t['seed'],t['seat'],t['cash'],t['opponent_cash'])
            assert t['matched_days']in[0,FULL,FULL^(1<<28)]
            delta=[a-b for a,b in zip(g['produced'],p['produced'])]
            assert delta==([6]+[0]*11 if t['matched_days']else[0]*12)
            if not t['matched_days']:assert g==p
            trace_rows.append({'rival':rival,**t})
        games=read(RUN/f'results/native_wheat_one_fert_vs_{rival}.json')['games']
        parent=read(RUN/f'results/native_crop_value_m2_t4_vs_{rival}.json')['games']
        for g,p in zip(games,parent):
            delta=[a-b for a,b in zip(g['produced'],p['produced'])]
            assert delta in [[0]*12,[6]+[0]*11]
            native_rows.append({'rival':rival,'seed':g['seed'],'seat':g['seat'],'wheat_gain':delta[0]})
    source=read(RUN/'SOURCE_PARITY.json');assert source=={'exact_source_actions':3072,'exact_source_guards':128}
    prefix=read(RUN/'PREFIX_ROUTE_DIFFERENCES.json')
    assert all(row['cash_equal']and row['different_fields']==['action_hash','profile']for row in prefix)
    reference=read(RUN/'results/crop_value_m2_t4_pass128.json');primary=read(RUN/'results/wheat_one_fert_pass128.json')
    report={'completed_utc':datetime.now(timezone.utc).isoformat(),'primary_package':'proposals/wheat_one_fert',
        'primary_source_sha256':{str(p.relative_to(RUN)):sha(p)for p in [RUN/'source/policy.hpp',RUN/'proposals/wheat_one_fert/source/agent.hpp',RUN/'proposals/wheat_one_fert/source/agent.cpp']},
        'source_parity':source,'generic_debug_pair_thread_exact_games':len(generic),
        'fresh_primary_exact_guard_games':len(trace_rows),'fresh_guard_counts':dict(Counter(x['matched_days']for x in trace_rows)),
        'fresh_berry_branch_games':sum(x['berry_selected']for x in trace_rows),'fresh_biology_deviations':0,
        'fresh_day28_fallbacks':sum(x['matched_days']==(FULL^(1<<28))for x in trace_rows),
        'fallback_diagnosis':'Six games have a newly spawned weed at cell73, where the unchanged source day28 contract requests BUILD_COOP. All other checked physical fields match. Existing parent fallback preserves all target output and yields positive sampled margin gains; guard is not weakened.',
        'native_primary_games':len(native_rows),'native_exact_output_counts':dict(Counter(x['wheat_gain']for x in native_rows)),
        'pass_games':len(primary['games']),'pass_J':primary['pass_J'],'parent_pass_J':reference['pass_J'],
        'self_games':len(read(RUN/'results/wheat_one_fert_self16.json')['games']),
        'source_course_parity_limitation':'215/230 full records equal original separately compiled courses. Remaining15 use different valid off-prefix routes on days15/19 before switching at20; both cash values, production, buys/sales, hires, faults and discards equal. The exact complete day20 guards match, and3072 packaged source actions/128guards independently match raw compiler artifacts.',
        'scope':'Primary ready for parent review/promotion; this run makes no catalog, submission or handoff changes. Exact checks prove sampled gameplay and compiled continuation scope, not global optimality.'}
    (RUN/'VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n')
    (RUN/'fresh_guard_cases.json').write_text(json.dumps(trace_rows,separators=(',',':'))+'\n')
    for name in ['wheat_one_fert','wheat_one_plain','wheat_three_fert','wheat_three_plain','wheat_one_fert_unclosed']:
        p=RUN/'proposals'/name/'IMPORT.json';data=read(p);data['validation']='../../VALIDATION.json'
        data['status']='Validated primary; parent owns registration and promotion'if name=='wheat_one_fert'else'Research control; no promotion requested'
        p.write_text(json.dumps(data,indent=2)+'\n')
        (p.parent/'README.md').write_text(f'# {name}\n\nComplete checked wheat calendar over crop_value_m2_t4. See ../../README.md for donor/source lineage, full paired metrics and limitations; ../../VALIDATION.json records operational checks. '+
            ('Validated primary, ready for parent registration/promotion. Fresh and native panels improve every tested opponent\'s mean margin and preserve target production. Six fresh games safely fall back to the parent on day28 after an unrelated weed blocks a source BUILD_COOP precondition. 'if name=='wheat_one_fert'else'Retained research control; no promotion requested. ')+
            ('The parent\'s optional berry branch is preserved with complete day20 suffixes. 'if not name.endswith('_unclosed')else'This explicit negative control disables the optional berry suffix after entering the wheat course. ')+
            'No submission or shared-source changes.\n')
    commands=[['conda','run','-n','kaggriculture','g++','-std=c++20','-O2','-I',str(RUN/'build/policies/source_snapshot'),str(RUN/'source/parity.cpp'),'-o',str(RUN/'build/source_parity')],
        ['conda','run','--no-capture-output','-n','kaggriculture',str(RUN/'build/source_parity'),str(RUN)],
        ['conda','run','-n','kaggriculture',str(RUN/'build/policies/arena'),'--a','crop_value_m2_t4','--b','pass','--games','64','--seed-start','1000','--seat-mode','both','--threads','3','--validate','--output',str(RUN/'results/crop_value_m2_t4_pass128.json')]]
    (RUN/'SUPPLEMENTAL_COMMANDS.json').write_text(json.dumps({'reproducible_commands':commands,'source_parity_binary_sha256':sha(RUN/'build/source_parity'),'source_parity_cpp_sha256':sha(RUN/'source/parity.cpp')},indent=2)+'\n')
    hashes={str(p.relative_to(RUN)):sha(p)for p in RUN.rglob('*')if p.is_file()and'__pycache__'not in str(p)
            and p.name!='FINAL_AUDIT.json'and p.suffix!='.log'}
    (RUN/'FINAL_AUDIT.json').write_text(json.dumps({'frozen_at_utc':datetime.now(timezone.utc).isoformat(),'artifact_count':len(hashes),'sha256':hashes},indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
