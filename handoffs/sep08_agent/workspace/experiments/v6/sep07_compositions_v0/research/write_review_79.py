from datetime import datetime, timezone
from pathlib import Path
import ast
import hashlib
import json

R = Path(__file__).resolve().parent
E = R.parent
latest = R / 'refresh_sep08_0408'
assert json.loads((latest / 'REFRESH.json').read_text())['player_games'] == 72
means = json.loads((latest / 'invariant_means.json').read_text())['means']
rows = json.loads((R / 'review_78_comparison.json').read_text())
for row in rows:
    key = {'SELL_weighted_hour': 'SELL_mean_hour', 'BUY_PRODUCT_weighted_hour': 'BUY_PRODUCT_mean_hour'}.get(row['metric'], row['metric'])
    row['global72'] = means[key]
assert len(rows) == 82
(R / 'review_79_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')


def functions(path):
    return {n.name: ast.dump(n, include_attributes=False) for n in ast.walk(ast.parse(path.read_text())) if isinstance(n, ast.FunctionDef)}


old = functions(R / 'refresh_0008/notebook_audit/destbreso/x-ray-your-agent.source.py')
new = functions(latest / 'notebook_audit/xray/extracted_cells.py')
assert old == new and len(new) == 42
now = datetime.now(timezone.utc).isoformat()
audit = {'completed_utc': now, 'inspection': 'Static source only; no notebook code or package/model downloads executed.',
    'xray': {'source': 'https://www.kaggle.com/code/destbreso/x-ray-your-agent', 'functions_ast_identical': 42,
        'decision': 'Existing replay-analysis notebook; no changed function implementation or new agent port.'},
    'periodic': {'source': 'https://www.kaggle.com/code/earnestgomer/python-periodic-table',
        'decision': 'Pi digits/chemical-element mapping and generic competition-package/model examples. No Kaggriculture observation/action policy identified; not a relevant strong-agent candidate.'},
    'files_sha256': {str(p.relative_to(E)): hashlib.sha256(p.read_bytes()).hexdigest() for p in (latest / 'notebook_audit').rglob('*') if p.is_file() and p.name != 'AUDIT.json'}}
(latest / 'notebook_audit/AUDIT.json').write_text(json.dumps(audit, indent=2) + '\n')
native = json.loads((E / 'runs/empty_sale_validation_sep08_001/NATIVE_ANALYSIS.json').read_text())
assert not native['gates']['all_parent_paired_margins_nonnegative']
text = f'''# Review79 — {now}

Due04:08; completed after fresh replay download/analysis and notebook audit.
Goal active through Sep10 00:47UTC. Best remains observed_sale_lead_start_216.
No blocker, promotion, Git or external submission. Previous interval produced
new exact diagnostics, a frozen incumbent candidate and a broader resource-route
comparison; it was progress, not a status-only continuation.

Empty-sale mode2 discovery4096 games:1024 old controls exact and1024 mode1/2
records equal. Direct parent114W4T10L,+230.26 mean margin. Own production unchanged
throughout; all eight opponent means improve. Frozen33-opponent fresh panel
uses2000000..2000511, both seats,67584games across candidate/parent. It is running.
No policy fitting on that panel. Parent README hash differed only by the exact
previously recorded post-promotion documentation amendment; all code hashes
matched, and the amendment was included before launching fresh games.

Native/PASS5632 games complete. All mean margins and utilities nondecrease;
Ahmed improves2/256wins. However the mandatory direct-parent per-game margin
gate FAILS: seed2004097 seat0 gains47 own cash and53 rival cash, reducing margin
by6. Production, sold totals, hires, faults and rival actions remain unchanged.
Do not promote current mode2 merely because its aggregate result is positive.
Trace the affected market slots and distinguish input-order movement from pure
non-input sale movement. New hypotheses must receive their own unused validation
pool. Operational/frozen checks and the broad panel remain unfinished.

Joint mixed-day routes001 complete576games,192exactoldcontrols,160operational,
and16exactdiagnostic games/48daily comparisons. p355 day14 leaves4feeds,7waters,
3fertilizations and other work after last actions while16wheat and9fertilizer
were still held immediately before that turn. Old compiler also has delivery
gaps, but this replacement is worse. Current route cost ignores earlier route
fertilizer collection/wheat harvest that can supply later tasks. Thirty new
route policies isolate cumulative-input cost, runtime withdrawals and scarcity
penalty across six farms. Keep complete dependent work and cross-route resource
exchange as unresolved requirements; these are not another arbitrary priority
constant search. No compiler strength promotion.

Fresh04:08 cohort:72 player-games from59 unique replays. Changed X-ray notebook
has all42 function ASTs identical to its prior audited version. The periodic-table
notebook contains chemistry/pi mapping and generic package examples, no relevant
game controller. No new promising agent port in these two sources.

All82 global metrics below now use this new cohort. Local256 remains the frozen
accepted-agent native panel fromReview70; unmatched cohorts do not establish
head-to-head leaderboard strength. Preserve large/family composition search,
shop/opponent valuation and full day compilation alongside incumbent improvements.
Next review04:28UTC; next replay refresh05:08. Final900000 remains unused.

| Metric | Global72 (04:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(R / 'review_79.md').write_text(text)
entry = f'\n{now}: Review79:fresh72 player-games/59replays update all82metrics; Xray42functionsunchanged, periodicnotebookirrelevant. Empty-slot native5632 allmeans/utilitiesnonnegative but parent per-game gate fails once(-6); no promotion. Fresh67584 stillrunning. Joint routes576/192controls/160ops/16diagnosticfullgames fail densework; cumulativeinput routes960comparison running. Best unchanged.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (E / name).open('a') as out:
        out.write(entry)
print('Review79 complete; next04:28UTC.')
