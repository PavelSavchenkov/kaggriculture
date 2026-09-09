"""Compare corrected cheap economics with complete matched C++ continuations."""
from datetime import datetime, timezone
from pathlib import Path
from statistics import mean
import argparse
import csv
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--folder')
parser.add_argument('--label',default='MATCHED')
args=parser.parse_args()
original=json.loads((RUN/'later_groups_30s/EXECUTION.json').read_text())
rows=[]
for case in original['cases']:
    name=case['name']
    path=RUN/(args.folder or ('matched_h1' if (RUN/'matched_h1'/name/'MATCHED_RESULT.json').exists() else 'matched_h2'))/name
    result=json.loads((path/'MATCHED_RESULT.json').read_text())
    status=json.loads((path/'STATUS.json').read_text())
    assert status['turns']==719 and status['status']=='full_game_and_all_day_endpoints_pass'
    days=list(csv.DictReader((path/'daily.csv').open()))
    assert len(days)==30 and all(x['exact']=='1' for x in days)
    cost=sum(int(x['hire_cost'])-int(x['parent_hire_cost']) for x in days)
    estimate=case['estimate']
    own=result['cash']-result['parent_cash']
    rival=result['rival_cash']-result['parent_rival_cash']
    row={'name':name,'seed':result['seed'],'matched_path':str(path.relative_to(RUN)),
        'rotations':estimate['rotations'],'extra_hire_cost':cost,'own_gain':own,'rival_gain':rival,'margin_gain':own-rival,
        'estimated_own':float(estimate['mean_own_gain']),'estimated_margin':float(estimate['mean_margin_gain']),
        'production_delta':[a-b for a,b in zip(result['produced0'],result['parent_produced0'])],
        'rival_production_delta':[a-b for a,b in zip(result['produced1'],result['parent_produced1'])]}
    row['labor_adjusted_own']=row['estimated_own']-cost
    row['labor_adjusted_margin']=row['estimated_margin']-cost
    row['own_residual']=own-row['labor_adjusted_own']
    row['margin_residual']=own-rival-row['labor_adjusted_margin']
    rows.append(row)
single=[r for r in rows if r['name'] in ['sheep_3','sheep_12','sheep_13']]
joint=next(r for r in rows if r['name']=='sheep_triple')
groups={'sheep_joint_extra_hire_cost':joint['extra_hire_cost'],
    'sum_single_extra_hire_cost':sum(r['extra_hire_cost'] for r in single),
    'joint_own_gain':joint['own_gain'],'sum_single_own_gain':sum(r['own_gain'] for r in single),
    'scope':'Same fixed scenario and source, matching added wool/fertilizer and displaced wheat. Economic interaction includes shared labor and market effects; this does not isolate labor alone.'}
report={'completed_utc':datetime.now(timezone.utc).isoformat(),'cases':rows,'sheep_groups':groups,
    'own_mean_absolute_error_before_labor':mean(abs(r['own_gain']-r['estimated_own']) for r in rows),
    'own_mean_absolute_error_after_labor':mean(abs(r['own_residual']) for r in rows),
    'margin_mean_absolute_error_after_labor':mean(abs(r['margin_residual']) for r in rows),
    'scope':'Six exposed matched counterfactuals in two fixed worlds; selected complete continuations from '+(args.folder or 'fixed-job h1/h2 solves')+'. Counts are not league wins or out-of-sample calibration.',
    'limits':['Calendars are retrospective. Runtime policy must preserve future parent branches and trade adaptation.',
        'Labor-adjusted values use measured labor, not a general learned labor predictor.',
        'Source estimator sampled32 future-shop sequences; realized world and revised final service differ.',
        'Fixed-job zero-extra-worker failures are restricted-model failures, not minimum-labor proofs.',
        'Rival production is unchanged, but rival cash response is still underpredicted; trade timing/quantities and shared prices require further decomposition.']}
(RUN/f'{args.label}_ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
body='''# Complete matched animal continuations

All six selected cow/sheep continuations execute719 transitions against the live
public router, validating every action and every daily farm/inventory endpoint.
The source is the current strongest empty_sale_slots_m2. These remain offline
fixed-calendar counterfactuals in two exposed worlds, not deployable league wins.

| Candidate | Own cash gain | Margin gain | Extra hire cost | Cheap own estimate | Estimate less actual labor | Residual |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
'''
for r in rows:
    body+=f"| {r['name']} | {r['own_gain']:+} | {r['margin_gain']:+} | {r['extra_hire_cost']} | {r['estimated_own']:.2f} | {r['labor_adjusted_own']:.2f} | {r['own_residual']:+.2f} |\n"
body+=f'''
Across these six cases, own-cash mean absolute error falls from
{report['own_mean_absolute_error_before_labor']:.2f} to
{report['own_mean_absolute_error_after_labor']:.2f} after charging measured labor.
This supports the cheap-economics hypothesis in this small matched study. It
does not establish a general labor predictor or accuracy on unseen farms.
Margin error after labor remains{report['margin_mean_absolute_error_after_labor']:.2f};
the live rival's cash response differs although all rival production stays equal.

The single cow adds15milk and13fertilizer, displacing10wheat and1carrot. Three
cows add45milk/39fertilizer, displacing31wheat/2carrots. Each sheep adds10wool
and8fertilizer, displacing8wheat. The sheep group triples these quantities.
Three sheep together cost{groups['sheep_joint_extra_hire_cost']} extra labor versus
{groups['sum_single_extra_hire_cost']} summed across the three separate changes;
own gain{groups['joint_own_gain']} exceeds their summed{groups['sum_single_own_gain']}.
Shared labor and market interactions both matter.

## What made the compiler finish

1. Keep full animal lifetimes, including already planned future animals, when
   calculating displaced output, feed, fertilizer, work and canceled purchases.
2. Place extra animal orders only at a funded hour; physical stock arrival alone
   does not prove the order can be paid.
3. Reduce lost output carried into later days. Cancel early sales against missing
   opening stock before assigning new output to a later sale. Cow day28 then
   solves in0.34..0.36seconds with no extra worker.
4. Preserve actual untouched empty/weed tiles. Copying the original trajectory's
   weeds silently required random weeds to appear/disappear without work.
   A constructed courier schedule exposed the two invalid passive targets.
5. Fix unchanged original jobs to their worker/hour, leaving changed jobs free.
   After correcting passive targets, all six final days solve in0.10..0.16seconds
   with2extra workers. Five also solve with1extra worker. Cow triple still uses2.
   Every selected physical schedule then passes the complete live-engine audit.

The original final-day control solves in0.072seconds with fixed jobs, while the
same solver without assignments remainsUNKNOWN after30seconds. The false passive
targets made restricted models immediately infeasible. Earlier unrestricted
timeouts were therefore not evidence that these compositions needed more labor.

## Reproduction and next work

prepare_lifecycles.py and estimate_v2.cpp define the corrected economic screen.
run_later_groups.py, run_terminal.py and run_suffix.py retain all prior attempts.
prepare_market_repair.py and prepare_passive_tiles.py preserve the two contract
corrections. run_fixed_hints.py records the failed/control comparison;
run_passive_hints.py records corrected0/1/2-worker physical solves.
run_final_audits.py executes those schedules against the live opponent;
matched_h1 and matched_h2 retain complete actions, guards, problems and daily
cash/labor profiles. Protocols contain exact commands and source/binary hashes.
All Python, builds and binaries run through the kaggriculture conda environment;
solver-linked binaries also use the persistent day_solver/with_runtime.sh.

Compiler_v7 includes passive-state repair; compiler_v8 additionally tries a
bounded fixed-job repair before unrestricted solving. integrated_30s completes
all six cases with unchanged source hashes; integrated_audit separately checks
every action and daily endpoint in all six complete games. Unrestricted search
can improve on the fixed-job labor count: its final day uses0extra workers for
cow9 and sheep13, and1 for cowtriple. Old fixed-job results remain recorded.

Next build full observation-driven continuations with the parent's tomato,
animal-family and day20berry choices preserved; retain current market adaptation
and compare against parent and diverse opponents on matching fresh scenarios.
No source-season seed, hidden inventory or future shop path may reach the policy.
Broader cold farms and mixed cow/sheep/goose/wait choices remain in scope.
'''
(RUN/f'{args.label}_RESULTS.md').write_text(body)
stamp=datetime.now(timezone.utc).isoformat()
line=f'\n{stamp}: {args.label} six-case matched audit. All719transitions and daily endpoints pass. Measured labor reduces own forecast MAE {report["own_mean_absolute_error_before_labor"]:.2f}->{report["own_mean_absolute_error_after_labor"]:.2f}; margin residual remains{report["margin_mean_absolute_error_after_labor"]:.2f}. Small exposed fixed-world result; no runtime/league promotion. See runs/animal_groups_sep08_001/{args.label}_RESULTS.md.\n'
for name in ['PROGRESS.md','IDEAS_LEDGER.md','PROFILING_LEDGER.md']:
    with (EXP/name).open('a') as f:f.write(line)
print(json.dumps({k:v for k,v in report.items() if k not in ['cases','limits','scope']},indent=2))
