from datetime import datetime, timezone
from pathlib import Path
import json

R = Path(__file__).resolve().parent
E = R.parent
now = datetime.now(timezone.utc).isoformat()
latest = R / 'refresh_sep08_0508'
fresh = json.loads((latest / 'REFRESH.json').read_text())
assert fresh['player_games'] == 72 and fresh['raw_unique_replays'] == 61
assert json.loads((latest / 'notebook_changes.json').read_text()) == []
means = json.loads((latest / 'invariant_means.json').read_text())['means']
rows = json.loads((R / 'review_81_comparison.json').read_text())
for row in rows:
    key = {'SELL_weighted_hour': 'SELL_mean_hour', 'BUY_PRODUCT_weighted_hour': 'BUY_PRODUCT_mean_hour'}.get(row['metric'], row['metric'])
    row['global72'] = means[key]
assert len(rows) == 82
(R / 'review_82_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
native = json.loads((E / 'runs/empty_sale_floor_validation_sep08_001/NATIVE_ANALYSIS.json').read_text())
assert all(native['gates'].values())
text = f'''# Review82 — {now}

Due05:08; fresh download/analysis completed05:14. Goal active through Sep10
00:47UTC, no blocker. Current accepted best still observed_sale_lead_start_216.
No Git, official catalog copy or Kaggle submission. This interval completed
new full-engine scheduling evidence, exposed guard selection and fresh native
checks; it was progress rather than a status-only continuation.

Floor-guard mode1 passes all5632 native/PASS numeric gates on unused2104000
seeds, including every parent paired margin. Direct native parent222/256wins,
mean margin+204.92. Other opponent utilities unchanged; all mean margins
nonnegative. The1024 operational checks and64 isolated rebuilt records pass,
with239 frozen dependencies. Broad69632-game fresh audit across34opponents is
still running. No policy change or fitting after freeze. Do not promote before
its full numeric gates and source-hash audit pass.

Day witnesses45/48initial cases solve with exact full-engine replay. All12
augmented/two-fewer-worker cases solve; median successful time0.185seconds.
Movement falls substantially: p355joint day14 from146to85moves while adding24
field actions and saving89 in hires. Its extra output is20wheat,3milk,2fertilizer.
The original greedy controller often splits many tiles across workers; solved
routes usually group work but retain selective cooperation. Both absolute
fixed ownership and unrestricted greedy hopping miss useful structure.

Three timeout retries at8seconds add one solved case(6.285s), still full-engine
exact. Two remainUNKNOWN: p362joint day16 original work/two fewer workers, and
augmented work/original workers. The augmented/two-fewer case already solves.
Do not interpret search failure as infeasibility or keep rerunning all45passes.
Existing precise stock/state template guards activate too rarely. New work
constructs a deterministic clock and own farm from observations so fixed route
programs can be checked against current stocks. The rival's private stocks are
zero placeholders; it assumes no rival trades and no random weeds during the
forecast. The first verifier compile missed iostream; the log is preserved and
that include is fixed. Observation/replay parity checks are running before use
in a candidate. No relaxed guard or complete new day policy is validated yet.

Fresh05:08 cohort:72player-games from61unique replays, no changed notebooks.
All82metrics below use this new cohort; local256 remains the frozen accepted
native panel. These unmatched populations support gap analysis only. Keep large
composition changes, fast economic ranking, general task compilation and current
opponent/replay learning in scope alongside the incumbent market improvement.
Nextreview05:28UTC; next replay/notebook refresh06:08UTC. Final900000 untouched.

| Metric | Global72 (05:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {row['metric']} | {row['global72']:.4f} | {row['local256']:.4f} |\n" for row in rows)
(R / 'review_82.md').write_text(text)
entry = f'\n{now}: Review82:fresh72player-games/61replays updateall82metrics,nochangednotebooks. Floor guard passes5632native/PASS+1024ops+64frozen;fresh69632pending. Day witnesses45initial+1retry=46/48uniqueexactcases,all12augmented/twofewer solve; movement146->85 onp355jointday14 plus24tasks. Observation-built clock/model verifier compile include fixed,paritypending. Best unchanged.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (E / name).open('a') as out:
        out.write(entry)
print('Review82 complete; next05:28UTC.')
