"""Record completed paired comparisons and the next composition priorities."""
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
EXP = HERE.parent
previous = (HERE / 'review_104.md').read_text()
table = previous[previous.index('| Metric |'):]
assert sum(line.startswith('| ') for line in table.splitlines()) == 84
stamp = datetime.now(timezone.utc).isoformat()
body = f'''# Review105 — {stamp}

The previous goal turn made progress: it collected the completed 36,864-game
animal/premium comparison, ran its paired analyzer, and updated the lineage.
Current accepted reference remains empty_sale_slots_m2; cold reference remains
early_melon_b98_m1. No promotion or submission. Full objective remains active.

Fresh eight-neutral results versus accepted parent: premium priority +1.038pp
utility/+126.02 mean margin; animal m1/m2 +0.049pp/+132.66/+164.96; combined
m1/m2 +1.086pp/+259.02/+286.84. The animal win interval includes zero; sale
priority accounts for nearly all added wins. Combined m2 versus m1 changes no
wins but adds27.81margin. Native neutral includes only teammate/Yusuke and no
win changes, so it does not establish broad native noninferiority. All sources
unchanged. Audit individual opponents, cash and tails before selection.

The q24 opening gains12.081pp against seven neutral opponents but loses100.89
mean margin; preserve its failure of the old positive-margin gate. Its453/512
wins against Yusuke address a real counter that defeats every animal/premium
variant. Next combine components and inspect King's cash/margin tradeoff. Use
new registered criteria and independent seeds for any eventual new selection.

Day23 unexpected weed38 now repaired by the day solver with zero additional
workers. Independent original-control action hashes/cash match; full719-turn
repair restores6wheat, +152owncash/+172margin. One/two extra workers also restore
the crop but lose81/458cash. This shows that adding service need not add labor:
reuse spare route capacity before estimating the next Fibonacci hire. Runtime
observed-state repair is next, followed by original-fixture parity and league.

Broader compositions remain necessary: advancing two already-planned geese
gained519cash, earlycow pair lost4145, mixedday10 lost5531. Separate market/rival
forecast errors from labor and crop displacement. Mixedday9's missing2wheat
still requires a transaction trace. Below-parent workforce search, care-cap
exceptions, new placement and independent cold farms remain in scope. Nagata's
dynamic controller is statically audited but not yet ported; Roger tapes remain
unexamined candidates. Do not replace broad search with opening-only tuning.

All82metrics below explicitly reuse refresh1308's72player-games/57replays and
the unchanged native256 local panel. No new global cohort is due until14:08UTC.
They are unmatched populations. Their crop-service/production/sale-time gaps
guide hypotheses, not causal conclusions. Final900000 remains unused. Next
review13:58UTC; next replay/notebook refresh14:08UTC.

'''
(HERE / 'review_105.md').write_text(body + table)
note = f'\n{stamp}: Review105. Completed36864 paired factor games; premium+1.038pp, animals+0.049pp, combined+1.086pp. Weed38 exact zero-extra-worker repair restores6wheat/+152cash; runtime integration next. Full objective active; accepted/cold references unchanged. All82 metrics reuse1308/global andnative256. Next13:58review/14:08refresh.\n'
for name in ('PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md'):
    with (EXP / name).open('a') as stream:
        stream.write(note)
print('Review105 written; 82 metrics explicitly carried from the prior cohort.')
