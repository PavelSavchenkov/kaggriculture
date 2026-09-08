"""Record the latest global cohort and the execution-repair decision."""
from datetime import datetime, timezone
from pathlib import Path
import json

RESEARCH = Path(__file__).resolve().parent
EXP = RESEARCH.parent
global_data = json.loads((RESEARCH / 'refresh_2302/invariant_means.json').read_text())
previous = json.loads((RESEARCH / 'review_63_comparison.json').read_text())
rows = []
for row in previous:
    key = row['metric']
    mapped = {'SELL_weighted_hour': 'SELL_mean_hour',
              'BUY_PRODUCT_weighted_hour': 'BUY_PRODUCT_mean_hour'}.get(key, key)
    rows.append({'metric': key, 'global72': global_data['means'][mapped], 'local256': row['local256']})
assert len(rows) == 82
(RESEARCH / 'review_64_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
text = f'''# Review 64 — {datetime.now(timezone.utc).isoformat()}

Due23:08UTC; recorded after completing the requested lineage summary. Current
reference remains rival_wool_context_v3. The preceding research turn made
progress: exact funding diagnosis, two completed repair screens and a new
public controller source. The intervening user-facing lineage answer completed
the requested report; it did not promote or alter policy state.

Original objective retained: composition search with fast economics, exact
execution and correction from the gap. Repairing failed purchases addresses
the observed gap between a selected composition and its realized farm. It does
not complete general valuation, placement, independent construction or search.

The exact native256 diagnostic runner matches all original profiled records.
Of30 additional wool activations,26 first reject a compiled day onday11:
20miss a sheep after an unaffordable two-sheep order;6have all animals but one
wheat too little. The earlier claim that all26missed sheep was corrected in
PURCHASE_CAUSE.json. Cash can recover before the scheduled pickup; the source
does not retry the missing purchase.

Combinedrepairv2: observe actual sheep delivery and retry before pickup253;
at263restore one missing portable wheat when no competing order or wheat-use
action exists and capacity/cash suffice. Recognize both early and delayed wool
contracts. No worker route or opponent-ID condition was added.

Completed3456profiled discovery games comparev2,v1,current across7opponents and
nativeV5/2. Six other opponents have all records unchanged. AgainstV5/2custom128,
v2 changes8games,meanmargin+$105.42,owncash+$82.92,faults-2.92,unchangedwins.
Native256:26change,meanmargin+$146.77,owncash+$47.97,faults-5.92,oneadditionalwin.
No paired margin worsens. Relative tov1, the wheat repair changes exactly6native
games, adding$18.45meanmargin and removing0.63faults. New broad validation will
use1970000..1972047bothseats against30opponents with the existing strict utility
gates; cash improvement alone will not be called a promotion.

Fresh23:02global cohort:72playergames fromcurrenttop12. Two changed notebooks:
Georgy's live meta report is analysis; Ahmed's V23 is a new reactive controller
on the already verified ThomasV5/2 donor. Static template decoded and hashed,
notebook not executed. It adds weed repair, sell lead, budget/room guards, sale
clamping, dead-stock sales and terminal liquidation. Its claimed win rates are
unverified. Next independent task: C++port, exact Python-source parity, then
league tests and component borrowing where measured useful.

Priority: fresh combined-repair validation while porting the new controller.
The general estimator and cold-construction gaps remain explicit. The latest
71-course full-policy screen was negative; no new leaderboard-rank claim.
Next review23:28UTC; next freshglobal00:02UTC. Final900000reserveunused.

All82metrics below compare the fresh23:02global72cohort with the unchanged
current reference's256nativegames againstitsparent. These are different cohorts,
not a paired estimate of strength against the top players.

| Metric | Global72 (23:02) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {row['metric']} | {row['global72']:.4f} | {row['local256']:.4f} |\n" for row in rows)
(RESEARCH / 'review_64.md').write_text(text)
print('Review64 records82metrics; next23:28UTC.')
