from datetime import datetime, timezone
from pathlib import Path
import json

R = Path(__file__).resolve().parent
E = R.parent
now = datetime.now(timezone.utc).isoformat()
rows = json.loads((R / 'review_80_comparison.json').read_text())
assert len(rows) == 82
(R / 'review_81_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
days = json.loads((E / 'runs/joint_day_witnesses_sep08_001/RESULTS.json').read_text())
assert len(days['cases']) == 48 and days['solved'] == 45 and days['all_solved_cases_full_engine_exact']
text = f'''# Review81 — {now}

Due04:48, written after the new day-witness results. Goal remains active through
Sep10 00:47UTC. The preceding turn was progress: it completed a frozen audit,
proved a causal floor-price regression and launched concrete guarded candidates.
This turn adds new exact compiler evidence. No blocker or strongest promotion.

Floor guards complete4096 discovery games,1024 exact prior controls and48
operational games. Mode1 guards only own requested sale volume near price1;
it retains+217.95 direct-parent mean margin versus+230.26 unguarded. The matched
rival-volume and100-unit guards retain+162.89 and+15.59. All eight discovery
opponents have nonnegative utility, mean margin and every paired margin with
mode1. Exposed witnesses1290games include512 exact native complete controls and
four custom endpoint/hash controls. All guards fix both known failed cases;
mode1 native mean gain+210.61 versus+219.64 unguarded. These are exposed tests.

Mode1 was frozen before using new pools2100000/2103000/2104000. Fresh comparison
is69632games across candidate/parent and34opponents, including the unaccepted
old empty-sale version. Keep the same strict parent/league gates. Native/PASS
and operational checks are running. Do not fit the policy on these results or
promote until every gate and isolated rebuild passes. Accepted reference remains
observed_sale_lead_start_216; no Git, catalog copy or external submission.

Joint day witnesses:four complete source games match both hashes and cash;
each original/joint pair starts day14 in the identical full state. Across
days14..16, original versus augmented work, and original versus two fewer hires,
45/48cases solve within2seconds. Every returned schedule passes exact full-engine
inventories, production, deterministic tiles, cash and zero unit-fault checks.
All12 augmented cases with two fewer hires solve. p355joint day14 adds7waters,
6harvests,3fertilizations,4feeds,2collections and2cares; gains20wheat,3milk and
2fertilizer while saving89cash. p362joint day14 adds8wheat,2fertilizer and service
while saving233. These extra products stay in inventories under fixed trades;
do not count them as realized extra cash yet. Construction tasks remain excluded.

The three unresolved cases are all p362joint day16; its augmented/two-fewer
version already solves, demonstrating that timeout does not prove infeasibility.
Retry only those three with more search time. More work with fewer hires proves
the generic routing loss is not simply insufficient workforce on these states.
Next: extract route/resource differences and turn plans into reusable programs
with observation-based entry checks and current-stock execution. Existing exact
stock/tile guards activate sparsely; blindly loosening them would lose safety.
Preserve general task generation and larger/family composition search; these
offline witnesses are not a complete branching season compiler.

All82 metrics below carry the04:08 fresh72-player-game cohort and accepted-agent
native256panel. No new external refresh since then. Cohorts are unmatched and
do not estimate head-to-head leaderboard strength. Nextreview/refresh05:08UTC.

| Metric | Global72 (04:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {row['metric']} | {row['global72']:.4f} | {row['local256']:.4f} |\n" for row in rows)
(R / 'review_81.md').write_text(text)
entry = f'\n{now}: Review81:floor guards4096/1024exactcontrols/48ops/1290exposedwitnesses selectmode1; new frozen69632fresh plusnative/ops running. Joint day witnesses45/48solve,allfullengineexact;all12 augmented/two-fewer-hirecases solve. p355jointday14 recovers20wheat3milk2fert plusserviceand89laborcost, fixedtrades. Compiler route loss not simply too few workers. Retryonly3timeouts; nextgeneralize programs with validated observation-based entry. Best unchanged;all82metrics carried04:08.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (E / name).open('a') as out:
        out.write(entry)
print('Review81 complete; next05:08UTC.')
