"""Review repaired animal policies and newly isolated forecast/funding errors."""
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
EXP = HERE.parent
old = (HERE / 'review_105.md').read_text()
table = old[old.index('| Metric |'):]
assert sum(x.startswith('| ') for x in table.splitlines()) == 84
stamp = datetime.now(timezone.utc).isoformat()
body = f'''# Review106 — {stamp}

Current accepted/cold references remain empty_sale_slots_m2/early_melon_b98_m1.
This continuation makes implementation and measured progress; no promotion.
Five new C++ packages combine the certified weed repair, animal selection,
premium-sale priority and quantity24 opening. Twelve old course fixtures remain
unchanged; two active repair pairs preserve animal output/workforce and restore
6wheat. Runtime fixed/live modes gain152/170cash, faults21->11. All56operations
pass, including active generic/pair/debug/thread parity and self/PASS.

Completed9856combination discovery games with498frozen dependencies unchanged.
Full q24/repaired-animal/premium combination wins121/128vs accepted,119/128vs
Yusuke,126/128vs teammate. Eight-neutral win score improves11.816pp; mean margin
+186.37 with interval[-89.54,604.82], owncash interval still straddles zero.
King loses3.125pp and9920margin; retain this tradeoff. Compared with premium/q24
alone, animal selection changes no wins but adds240.44mean margin with positive
lower bound22.63. Next broad independent/native audit of the full combination,
no-q24 combined challenger, and accepted parent, including former versions and
new public ports. Do not promote from exposed discovery or relabel old q24 gates.

Two full-course forecast diagnoses now reproduce original hashes and cash.
Earlycow conditional-after-labor own/margin=-2088/+3092. Actual future shops
change this to-5889/-1125. Actual own daily trades change own estimate only44;
actual rival daily trades change it1759 and leave29cash/215margin residual.
Actual geese daily-flow residual is13cash/8margin. Substitution order matters;
do not claim independent additive causal effects. Unknown shops and incomplete
rival forecasts matter more than intraday timing in the earlycow witness.
Existing public-current-crop helper was re-read: prior broad gains were small
and uncertain; short-crop renewal, future animals and fertilizer remain missing.
Next evaluate observation-only rival growth/disposition models across contexts.

Mixedday9 shortage is now exact: at hours4/6, requested3wheat but bought2 each,
with cash76/89 and price34. Late2wheat at hour23 costs72 and restores end stock
37, all other stock/seeds and farm tiles unchanged. Certify the revised physical
contract and resume full-season compilation before calling this a viable farm.
No new labor is needed for this financial repair.

Broader/cold/larger/mixed farms, below-parent hiring and care-cap exceptions remain
in scope. Nagata port and Roger tapes are queued public alternatives. No GPU
workload needed; no Git, catalog change or external submission. Final900000unused.
All82metrics below explicitly reuse1308global72/57 and currentnative256; no new
cohort yet. Nextreview14:18UTC; replay/notebook refresh14:08UTC.

'''
(HERE / 'review_106.md').write_text(body + table)
note = f'\n{stamp}: Review106. Repaired/composed agents16fixture+56ops+9856discovery pass; full q24combo+11.816pp/+186.37margin, King tradeoff persists. Forecast actual-shops/rival-flow diagnostic isolates major earlycow errors. Mixedday9 partial wheat buys traced and late stock correction verified; full certificate/season pending. All82metrics reuse1308/native256. Next14:18/refresh14:08.\n'
for name in ('PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md'):
    with (EXP / name).open('a') as stream:
        stream.write(note)
print('Review106 written with82 carried comparison metrics.')
