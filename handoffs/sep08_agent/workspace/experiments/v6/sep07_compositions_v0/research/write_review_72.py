from datetime import datetime, timezone
from pathlib import Path
import json

RESEARCH=Path(__file__).resolve().parent
EXP=RESEARCH.parent
rows=json.loads((RESEARCH/'review_71_comparison.json').read_text())
assert len(rows)==82
(RESEARCH/'review_72_comparison.json').write_text(json.dumps(rows,indent=2)+'\n')
now=datetime.now(timezone.utc).isoformat()
text=f'''# Review72 — {now}

Due01:48; recorded after checking whether the compiled first-day schedules
actually execute in complete games. Best remains observed_sale_lead_start_216;
full objective stays active through Sep10 00:47UTC. This interval made concrete
progress in placement/financing diagnosis and direct task generation.

Placement study completed256games,64 exact old controls,64 unchanged dairy/wool
records andtwo exact full-game traces. Owned-first mixed placement buys sheep
onday0 instead ofday8 in all32 games. The witness moves its$1000land purchase
fromstep0 to168, so those funds buy the two sheep initially. Public opponent
owncash+$1660.375, margin+$4682.625; current opponent owncash+$2264.75,
margin+$6373.375. More wool/fertilizer comes with more walking and less wheat.
Goose saves land cost while producing fewer eggs/fertilizer. Both remain weak.

New cold_day_tasks_sep08_001 generates initial-day work from raw dated lives and
owned-first placement:19crop plant/water pairs,4animal structures/placements,
explicit service choices, fixed purchases and8hires. No source worker routes or
assignment hints enter the day problem. All three cases solve in51–67ms and
match the root full engine's next-morning tiles, stocks, seeds and zero faults
againstPASS. Physical day feasibility is still distinct from economic value.

Three complete C++ policies plus an unchanged explicit-layout control ran768
profiled games againstpublic_router,current andPASS. First-day improvements
are only about$1–$100cash. A separate96-game observer reproduces complete prior
records and confirms every game executes all24 compiled hours; none relies on
the unit-count fallback. Earlier concern about hidden fallback activation is
therefore resolved for that measured panel. Skipped initial service saves wheat
without changing animal output in this continuation, but better later service
could change its value. Do not claim biological equivalence from this result.

Operational generic/pair/debug/self/PASS/thread checks are running in86810.
The paired runner's initial command omitted output agent labels; the static
pair is correct, but its result metadata must be corrected by a labeled rerun
after completion, preserving the original. This does not change policy code.
No cold candidate is promoted as a strong agent.

Next: later-day task construction and exact resource/finance constraints, then
full-season error feedback. Do not continue tuning a first day that explains
little of the remaining gap. Connect placement explicitly to the cheap model;
keep larger/independent compositions, observed-shop/opponent choices and the
growing league. The original request includes a strong arbitrary composition
compiler, not just a successful initial-day example.

Latest external cohort remains01:08; all82 current/global metrics below are
explicitly carried forward fromReview70. Different cohorts are not a paired
top-player strength estimate. Next review02:08 and hourly refresh02:08.
Final900000 remains unused. No Git, officialagentcopy or Kaggle submission.

| Metric | Global72 (01:08) | Current native256 |
| --- | ---: | ---: |
'''
text+=''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(RESEARCH/'review_72.md').write_text(text)
entry=f'\n{now}: Review72: placement financing confirmed in full traces;768 full games of task-generated day0 and96 exact coverage games. All24 schedule hours execute. Small cash gains, no strong promotion. Operational86810 finishing; pair result-label rerun required. Nextreview/fresh02:08; fullgoalSep10.\n'
for name in ['IDEAS_LEDGER.md','PROFILING_LEDGER.md','PROGRESS.md']:
    with (EXP/name).open('a') as file:file.write(entry)
print('Review72 recorded; next02:08UTC.')
