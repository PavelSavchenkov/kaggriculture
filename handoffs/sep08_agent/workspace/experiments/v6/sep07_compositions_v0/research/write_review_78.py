from datetime import datetime, timezone
from pathlib import Path
import json

R = Path(__file__).resolve().parent
E = R.parent
rows = json.loads((R / 'review_77_comparison.json').read_text())
assert len(rows) == 82
(R / 'review_78_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
now = datetime.now(timezone.utc).isoformat()
text = f'''# Review78 — {now}

Due03:48. Active goal through Sep10 00:47 UTC; no blocker. Best remains
observed_sale_lead_start_216. No promotion, Git or submission.

Arlene V4 complete:8628 exact public-source actions,8960 full discovery games,
1792 exact existing capacity/terminal controls,80 operational games. Full port
wins15/128 against incumbent and has no utility gain over its base across seven
opponents. The separate budget bit preserves all896 base records. Clamp bit2
loses about25k mean margin against V5/2 and Ahmed. Eight traced games reproduce
both action hashes and cash exactly. Zero SELL shifts a later wheat BUY; first
cash difference is step5 (-3 own/+3 rival); first workforce difference step25
is one versus three successful hires. This is a real production/finance loss,
not merely a different representation of a no-op. Full source and failures are
retained. Test empty-slot removal on incumbent next; late non-input-only mode
preserves its early funding. Three policies plus incumbent control,4096 games,
are running on exposed discovery seeds. No fresh promotion claim.

Partial-load routes002 complete256 games/128 exact old-route controls/80ops and
64 exact coverage games. p355 positive-wheat reversals fall42 to0, but cash changes
less than2 dollars on each opponent mean. The visible loop was real but was not
the main loss. Stop optimizing this narrow late-only component.

New joint-day routes001 expose the existing compiler's legal tile tasks, input
requirements and dates, then assign mixed crop/animal/construction tiles and
withdrawals to workers.576 full games preserve192 old controls. Small mixed
cash gains120-335, but dense and goose farms lose severely. p355 day14 start
adds11 eggs and13.75 wool while losing49.25 strawberries and13.875 milk, with
unchanged hires against public. General route replacement rejected. Diagnose
the first changed mixed day and blocked inputs/dependent tasks against an exact
day-solver schedule; do not simply retune job priority constants.

The original objective remains larger than these compiler prototypes. Preserve
dated composition/family search, realistic finance and labor estimates, observed
shop/opponent choices, fresh public borrowing and broad incumbent comparisons.
Exact solver schedules already prove some missed feed and labor are avoidable;
the reusable compiler still fails to realize that feasibility across states.

All82 metrics are carried fromReview76: fresh03:08 global72 and accepted native256.
These are unmatched cohorts. Next review and fresh top replay/notebook refresh
04:08UTC. Final900000 remains unused.

| Metric | Global72 (03:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(R / 'review_78.md').write_text(text)
entry = f'\n{now}: Review78:Arlene8628 source parity/8960games/1792exactcontrols/80ops; clamp losses traced to early market-slot cash/hires. Incumbent empty-slot4096 comparison running. Partial routes remove42reversals but score neutral. Joint mixed routes576games/192exactcontrols trade crop output for animal service and are rejected; first-day task diagnosis next. All82 metrics retained. Best unchanged.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (E / name).open('a') as out:
        out.write(entry)
print('Review78 complete; next04:08UTC.')
