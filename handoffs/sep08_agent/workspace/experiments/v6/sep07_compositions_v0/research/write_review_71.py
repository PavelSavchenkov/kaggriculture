from datetime import datetime, timezone
from pathlib import Path
import json

RESEARCH=Path(__file__).resolve().parent
EXP=RESEARCH.parent
rows=json.loads((RESEARCH/'review_70_comparison.json').read_text())
assert len(rows)==82
(RESEARCH/'review_71_comparison.json').write_text(json.dumps(rows,indent=2)+'\n')
now=datetime.now(timezone.utc).isoformat()
text=f'''# Review71 — {now}

The previous goal turn made progress: verified lineage evidence and a completed
user report. This research interval completed two independent compiler studies,
fresh external retrieval and all82 current/global metrics. The best remains
observed_sale_lead_start_216. The full goal stays active through Sep10 00:47UTC.

Hiring study:512 complete discovery games plus128 controls,64 copied-mode0 full
records exact. More hands recover some output, but no compiler wins. Eight
hands raise cold mixed cash$38110->$44770. The dated estimator's raw workforce
raises output on source4/55 but costs$26122/$34523. Source hiring still leaves
source55 cash$60337 versus$97655 under its original schedule, with the same
280hires/$5400. Its compiler uses3981moves versus3200; source4 uses4339vs3562.

Routing-score study:512 complete games and64 exact full control records. Giving
current-tile work a bonus reduces moves but changes the allocation of service:
source55 gets more strawberries/fertilizer and less milk/wool. Source55 cash
improves about$1972 while source4 regresses about$2249. Same-tile bundle values
do not produce a general gain. Do not continue arbitrary priority-constant
tuning. The failed persistent-target evidence from the earlier window agrees
that smaller movement counts alone do not establish better execution.

Next placement study is live (session39714). The distance/quadrant score can
buy adjacent land while owned tiles remain unused. In the cold mixed case,
sheep requested onday0 arrive aroundday8. Test filling allocated quadrants
first, keeping the mixed farm's eight hands fixed. Goose/dairy/wool also test
the placement, with the same estimator applied to their changed layout. Those
three may legitimately change estimated labor and are not a placement-only
causal control. Exact early orders and actual births must establish the
funding mechanism before claiming the hypothesis is correct.

The day-solver public interface was reread. It accepts exact tile work and
day-end states, fixed buys/hires and output withdrawals. It does not solve cash,
market prices, capacity or arbitrary mid-day entry. The next general compiler
design should generate complete daily task/resource constraints and validate
the resulting schedule in full games. A wrong economic endpoint must not be
called a solver failure. Likewise UNKNOWN is not proof of infeasibility.

The user's original composition-first ideas remain the guide: cheap dated
biology/placement/labor/economics, learn productive practices from top players,
choose observed-shop/opponent continuations, compile only useful candidates,
feed exact errors back into estimates and execution, preserve cold and large
family edits, and maintain the growing league. These studies expose concrete
placement/finance/labor errors. They do not complete the general compiler or
replace the objective with a weak cold-farm demonstration.

Latest external cohort remains72 from01:08, and no best-policy change requires
new native games. All82 metrics below explicitly carry forward Review70's NEW
accepted-reference native256 and freshglobal72. They are different cohorts,
not a paired top-player win-rate estimate. Next review01:48, refresh02:08.
Final900000 unused; no Git, official catalog copy or Kaggle submission.

| Metric | Global72 (01:08) | Current native256 |
| --- | ---: | ---: |
'''
text+=''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(RESEARCH/'review_71.md').write_text(text)
entry=f'\n{now}: Review71: completed hiring640 and route512 games with128 full mode0 parity records; no generic-compiler promotion. Stop priority tuning. Placement/funding controlled study running39714. All82 current/global metrics explicitly carried forward. Next review01:48, refresh02:08; full goal throughSep10.\n'
for name in ['IDEAS_LEDGER.md','PROFILING_LEDGER.md','PROGRESS.md']:
    with (EXP/name).open('a') as file:file.write(entry)
print('Review71 recorded; next01:48UTC.')
