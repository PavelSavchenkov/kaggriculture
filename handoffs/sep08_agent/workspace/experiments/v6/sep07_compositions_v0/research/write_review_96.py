from pathlib import Path
from datetime import datetime, timezone
import json

R=Path(__file__).resolve().parent
E=R.parent
now=datetime.now(timezone.utc).isoformat()
meta=json.loads((R/'refresh_sep08_1008/REFRESH.json').read_text())
means=json.loads((R/'refresh_sep08_1008/invariant_means.json').read_text())['means']
rows=json.loads((R/'review_93_comparison.json').read_text())
assert len(rows)==82
aliases={'SELL_weighted_hour':'SELL_mean_hour','BUY_PRODUCT_weighted_hour':'BUY_PRODUCT_mean_hour'}
for row in rows:row['global72']=means[aliases.get(row['metric'],row['metric'])]
(R/'review_96_comparison.json').write_text(json.dumps(rows,indent=2)+'\n')
body=f'''# Review96 — {now}

Concrete progress; no blocker. Global empty_sale_slots_m2 and cold
early_melon_b98_m1 remain. Goal throughSep10 00:47UTC; final900000 unused.

Corrected group screen:281256proposals/18scenarios,4242exact crop lifetimes,
317exact animal lifetimes and36full source/control games.14.695estimator seconds,
52.25us per proposal including32conditional shop samples. Initial screen omitted
already planned animals on some released melon tiles; those scores are invalid.
The compiler also duplicated their build/place tasks. New full-lifecycle model
subtracts their output, feed, fertilizer, work and cancelable future buys. Sunk
animal stock is retained. Goosepair estimate2450.69->1032.59; original failed
artifacts retained. Scores still use retrospective calendars and unpriced labor.

Actual406cash bought1 of2requested cows. Funded purchase insertion fixes day8
with no extra hire; day9 passes with1. Earlyday10 remainsUNKNOWN30s.42strict
source-day controls pass with noerrors after removing duplicate explicit sales
from the physical checker.10nohint/softhint light-exact comparisons yield no
schedules; originalday10 has an exact known-feasible witness. Duplicated-goose
models are malformed, not proof that intended replacement is impossible.

28isolated engine/model cases show a fully serviced cow can omit care on
birth+1 andbirth+2 with identical daily milk/fertilizer, saving2field actions.
The initial oracle forgot daily feed pickup; failure and correction retained.
No full-farm profit claim follows.

Later sheepday20 and cowday15 groups plus singles are now compiling. Three sheep
singles reachday28 exactly, then fail to find day29 schedules. Cow single reaches
day26 and then requires unavailable carrot inventory from displaced crops.
Next correct the continuation's inventory targets and final useful service,
reusing certified prefixes; finish matched economics before a runtime policy.

Fresh72player-games/{meta['raw_unique_replays']}replays update all82global metrics below. Local256
is unchanged; unmatched populations, no causal comparison. Two notebooks audited
statically: funding experiment discloses donor0/48wins againstAhmedV23, so no
port prioritized; Salem contains720-action8cow/4sheep course with weed replay and
next-turn sales. Source/tape decoded for C++ conversion; donor score claims are
not our validation. No notebook/policy code executed in this audit.

Next review10:28; next replay/notebook refresh11:08. No Git, upload, official
catalog, subagents or GPU workload changes. Goal remains active.

| Metric | Global72 (10:08) | Current native256 |
| --- | ---: | ---: |
'''
body+=''.join(f"| {x['metric']} | {x['global72']:.4f} | {x['local256']:.4f} |\n" for x in rows)
(R/'review_96.md').write_text(body)
line=f'\n{now}: Review96:corrected281256estimates/4242crop+317animal lifetimes/36controls;42strictdaycontrols;10hintcomparisons no schedules;28careequivalences. Sheep singlesday28exact, cowday26exact then inventory-target error. Fresh72/{meta["raw_unique_replays"]} plus2staticnotebookaudits updateall82metrics. Salem C++port pending. Next10:28/11:08; bestunchanged.\n'
for name in ['PROGRESS.md','IDEAS_LEDGER.md','PROFILING_LEDGER.md']:
    with (E/name).open('a') as f:f.write(line)
print('Review96 complete',meta['raw_unique_replays'])
