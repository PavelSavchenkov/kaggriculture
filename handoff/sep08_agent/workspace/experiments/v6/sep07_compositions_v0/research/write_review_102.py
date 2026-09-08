"""Review fresh top-player measurements and the current animal policy work."""
from datetime import datetime, timezone
from pathlib import Path
import json

HERE=Path(__file__).resolve().parent
EXP=HERE.parent
fresh=json.loads((HERE/'refresh_sep08_1208/invariant_means.json').read_text())
assert fresh['games']==72
rows=[]
for line in (HERE/'review_101.md').read_text().splitlines():
    if not line.startswith('| '):continue
    cells=[x.strip() for x in line.split('|')[1:-1]]
    if cells[0] in ('Metric','---'):continue
    key=cells[0].replace('_weighted_hour','_mean_hour')
    rows.append((cells[0],fresh['means'][key],float(cells[2])))
assert len(rows)==82
stamp=datetime.now(timezone.utc).isoformat()
body=f'''# Review102 — {stamp}

The immediately preceding user-requested lineage summary was read-only: no
optimization progress in that turn. This continuation revalidated completed
library-export session84282 and collected its terminal success, then created
the runtime-course audit and launched required operational builds. Goal remains
active; current reference is empty_sale_slots_m2, cold reference early_melon_b98_m1.

Animal library contains 190 compiled days: crops versus one/three cows at day15,
and crops versus three sheep placements or their group at day20, with two audited
contexts each. Twelve investment construction cases and four baseline exports
have complete original-game evidence. Runtime policies now exist in seven C++
packages, but competitive validation is pending. Forced-course audit checks all
719 turns, expected final cash for both players, activation and every daily guard.
Mode0 is parent control; mode1 retains compiled market orders; mode2 adds observed
inventory sale anticipation. Missed guards continue a course and are diagnosed;
this does not yet establish safe repair. Entry coverage, actual production and
service must be measured before treating the new selector as complete.

The broader scope still includes earlier entries, other farm families, mixed
species, placement changes and cold construction. These first library cases are
a way to evaluate the composition hypothesis, not a replacement for that scope.
Use compiled labor in valuation and compare prediction with actual cash/margin;
the negative alternate cow world proves unconditional investment is unsuitable.

Opening q13/q20/q24 discovery is complete: all turn Yusuke0/128 into113/128,
but King regresses and q13/q20 also regress Bohann. No promotion. Yusuke remains
a useful counter, with1024/1024 direct wins but weaker six-neutral utility:
87.394% versus current94.189%. Keep both in the league.

Global refresh1208 completed72player-games/55unique replays. All six changed
notebooks were downloaded and cells extracted without execution. AhmedV24 is a
promising small change over the verifiedV23: stable positive non-input sale
priority fromstep144. Source credits KingRC4's ordering idea; its self-reported
offline results are unverified here. Static diff and local component tests next.
Other five snapshots still need content-level audit; do not count downloads as
learning or ports. Next review12:48UTC and next global refresh13:08UTC.

All82global means below use the new1208cohort. Local values reuse the unchanged
native256 panel from review101. These populations are unmatched and cannot prove
a causal leaderboard gap. Local crop yield-day service remains lower, tomato
output lower and sale time later; inspect profitable calendars and same-world
effects before prescribing more service or more tomatoes. Final900000 unused.

| Metric | Global72 (12:08) | Current native256 |
| --- | ---: | ---: |
'''
body+=''.join(f'| {name} | {global_value:.4f} | {local_value:.4f} |\n' for name,global_value,local_value in rows)
(HERE/'review_102.md').write_text(body)
with (EXP/'PROGRESS.md').open('a') as f:
    f.write(f'\n{stamp}: Review102 updates82metrics fromfresh1208. Runtime animal library exported190days/12courses/7packages; exact fixture audit and operational builds in progress. Current/coldreferences unchanged. Last lineage-summary turn was read-only; this turn resumes implementation. Next12:48/13:08.\n')
print('Review102 written with82freshglobal metrics.')
