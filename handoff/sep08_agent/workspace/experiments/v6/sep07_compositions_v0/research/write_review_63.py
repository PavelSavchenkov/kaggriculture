"""Record the promoted rival response and all current global/local metrics."""
from datetime import datetime, timezone
from pathlib import Path
import json
import statistics

RESEARCH = Path(__file__).resolve().parent
EXP = RESEARCH.parent
path = EXP / 'runs/rival_wool_validation_003/native/rival_wool_context_v3_vs_wool_family_context_v2.json'
games = json.loads(path.read_text())['games']
means = json.loads(path.with_suffix('.profile.json').read_text())['means']
previous = json.loads((RESEARCH / 'review_61_comparison.json').read_text())
rows = []
for row in previous:
    key = row['metric']
    if key.startswith('harvest_'):
        value = means[key.replace('harvest_', 'produced_')]
    elif key == 'unit_faults':
        value = means['faults']
    elif key in ['SELL_weighted_hour', 'BUY_PRODUCT_weighted_hour']:
        field = 'sell_hours' if key.startswith('SELL') else 'buy_hours'
        values = []
        for game in games:
            hours = [sum(products) for products in game['profile'][field]]
            assert sum(hours) > 0
            values.append(sum(hour * count for hour, count in enumerate(hours)) / sum(hours))
        value = statistics.mean(values)
    else:
        value = means[key]
    rows.append({'metric': key, 'global72': row['global72'], 'local256': value})
assert len(rows) == 82
(RESEARCH / 'review_63_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
text = f'''# Review 63 — {datetime.now(timezone.utc).isoformat()}

Due22:48UTC. Promoted rival_wool_context_v3 at22:49. Goal continues through
September8 00:47UTC. No submission or Git. Previous goal turn made progress:
completed71-course replay screen and its analysis; this turn adds a validated
public-opponent response and an exact explanation of the failed preceding rule.

The original goal remains composition first, fast valuation before exact worker
planning, and correction from realized outcomes. The new rule selects a complete
farm from observed rival behavior. It adds a useful response, but does not replace
general raw-composition estimation, placement or independent construction.

514 diagnostic games verified the public cash discriminator. In John1930150seat1,
the attempted fertilizer sale fails; cash167→115 equals20hiring+32wheat, with no
seed spend. All30 relevant V5/2 contexts spend140on seeds beyond hires and wheat.
The source-copy revision uses observed actual hires, net product flows after
known demand, and positive residual spending. No opponent ID or hidden inventory.
Net flows are still ambiguous for arbitrary gross trade patterns.

Fresh57344games across28opponents pass all predeclared targeted-response gates.
V5/2 gains6.8359375pp and$182.14 mean margin. Currentgroup94.1899→94.4747%,
95%gain+0.1953..+0.3825pp. Historical utility exactly unchanged. All1024 complete
records against each other27opponents equal the parent; againstV5/2,110change.
No direct-parent gain is claimed: its1024 parent-self records remain exact.

Native5120 profiled games also pass. AgainstV5/2, utility78.90625→86.328125%,
meanmargin+$169.98, worst-decile margin+$885.88. However owncash-$346.21,
rivalcash-$516.19, hires+1.07 and unitfaults+8.47pergame. Rival actions remain
unchanged; shared prices explain the relative gain. The30 activated games show
large crop/animal changes and some broken execution. This is the next concrete
compiler/finance target, rather than another narrowing of shop contexts.

All1024 generic/pair/debug/thread/self/PASS operational comparisons pass, including
9custom and5native active cases. An isolated-link failure exposed two omitted
inherited source files in the new manifest. Restored both; every policy source
unchanged and the broad arena already linked them. The original registration and
PACKAGING_AMENDMENT.json preserve the repair.302actual C++dependencies frozen;
the rebuilt executable matches all256native fullrecords againstV5/2,30active.

Fresh71-course screen remains negative for full-policy replacement. Course26
(Tarang222) has only22.6faults and0.03discards pergame across tested opponents,
so its economic composition can be studied without a general execution collapse.
Course51(JustinLee) has889faults againstour opening but27againstV5/2, exposing an
opponent-dependent funding/execution failure. Keep these as separate mechanisms.

Next: trace current new-family failed orders and guard rejection days in the
activated native witnesses. Repair input/funding or recompile the actually
reached day contracts, then compare production, cost, own/rival cash and league
strength. Retain the broader cold-family and estimator work; do not substitute
an unmeasured labor model for full-game evidence. New global refresh23:02UTC;
next review23:08UTC. Final900000 reserve remains unused.

All82 metrics below use the unchanged22:02 global72 cohort and the newly promoted
agent's native256games againstitsparent on1954000..1954127. Cohorts differ; these
cash figures do not estimate paired leaderboard strength. The extra response
does not activate againstthisparent, so use the separateV5/2causal audit above.

| Metric | Global72 (22:02) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {row['metric']} | {row['global72']:.4f} | {row['local256']:.4f} |\n" for row in rows)
(RESEARCH / 'review_63.md').write_text(text)
print('Review63 records', len(rows), 'metrics; next23:08UTC.')
