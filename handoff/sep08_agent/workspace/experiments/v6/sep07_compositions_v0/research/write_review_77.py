from datetime import datetime, timezone
from pathlib import Path
import json

R = Path(__file__).resolve().parent
E = R.parent
rows = json.loads((R / 'review_76_comparison.json').read_text())
assert len(rows) == 82
(R / 'review_77_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
now = datetime.now(timezone.utc).isoformat()
text = f'''# Review77 — {now}

Due03:28; written after source parity and route coverage diagnosis.
Best remains observed_sale_lead_start_216. Goal active through Sep10 00:47UTC.
No blocker, promotion, Git or external submission. This interval made progress:
fresh notebook code was distinguished from inactive additions, a complete new
public combination was ported, and a concrete route failure was measured.

Arlene V4 full C++ mask31 matches8628 source actions:5752 recorded and2876 forced
route/budget/capacity/terminal fixtures, all four routes. The budget guard changes
11 requests across120 block checks;391 zero-sale slots are exercised. First
parity failed at step625 because pickup-aware projection was omitted; complete
act diff also found padded-worker weed repair. Failed source and logs are saved,
then both source features were added as separate masks. No completed strength
result predates that correction. Eight ablations plus two existing public
controls now run8960 profiled games across seven opponents. Operational checks
initially started before their generic build record existed; their runner now
builds its own generic control and is rerunning. No policy change from that
orchestration correction. Source parity alone does not establish strength.

Herd routes001 complete256 discovery,128 exact old controls,80 operational games,
and64 full-record-exact coverage games. Small mixed/goose farms activate all64
late days per family; p355 activates32days in8/16games; p362 activates none
against public because live crops remain. On p355,42 positive-wheat reversals
occur. Seed1000 seat0 worker8 steps678-680 alternates shed/adjacent cell with
one wheat because it cannot obtain the complete route load. New routes002
tests partial-load progress while finishing the current animal's service before
restocking, keeping old mode1 controls. It remains a narrow late-game repair.

Next general compiler work must address dense mixed crop/animal days: generate
dated tile tasks and resource requirements together, then assign routes that
preserve planting/water/fertilizer/feeding dependencies. Current late-only work
cannot fix day18 feed losses and must not become the whole research goal.
Keep broad composition changes, shop/rival valuation and direct incumbent
component improvements in scope. Reject universal smaller pickups and simple
priority retuning based on the completed negative studies.

All82 global/current metrics below are carried fromReview76: fresh03:08 global72
versus frozen accepted-agent native256. They are unmatched cohorts, not a direct
leaderboard comparison. Next review03:48; next replay refresh04:08. Final900000
remains unused.

| Metric | Global72 (03:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(R / 'review_77.md').write_text(text)
entry = f'\n{now}: Review77:Arlene V4 full mask31 matches8628 source actions/all4routes/11budget changes;8960 discovery and operational checks running. Herd routes001 complete256games/128exactcontrols/80ops/64coverage; partial-load return loop witnessed, isolated routes002 test running. All82 metrics retained from03:08 cohort. Best unchanged, main goal active.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (E / name).open('a') as out:
        out.write(entry)
print('Review77 written; next03:48UTC.')
