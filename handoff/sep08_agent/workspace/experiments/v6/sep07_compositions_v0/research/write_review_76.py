"""Record the fresh notebook audit and all 82 cohort comparisons."""
from datetime import datetime, timezone
from pathlib import Path
import ast
import hashlib
import json

RESEARCH = Path(__file__).resolve().parent
EXP = RESEARCH.parent
LATEST = RESEARCH / 'refresh_sep08_0308'
audit_dir = LATEST / 'notebook_audit'
router_notebook = next((audit_dir / 'router').glob('*.ipynb'))
cells = json.loads(router_notebook.read_text())['cells']
sources = [''.join(c['source']) for c in cells if c['cell_type'] == 'code']
source = next(s for s in sources if s.startswith('%%writefile main.py'))
router_source = source.split('\n', 1)[1]
(audit_dir / 'router/reference.py').write_text(router_source)


def functions(source):
    tree = ast.parse(source)
    return {n.name: ast.dump(n, include_attributes=False)
            for n in ast.walk(tree) if isinstance(n, ast.FunctionDef)}


original = functions((EXP / 'league/public_router/upstream_reference.py.txt').read_text())
router = functions(router_source)
assert all(router[name] == value for name, value in original.items())
tree = ast.parse(router_source)
calls = [n.func.id for n in ast.walk(tree)
         if isinstance(n, ast.Call) and isinstance(n.func, ast.Name)]
assert '_rank_sell_orders' not in calls
arlene_source = (audit_dir / 'arlene/decoded_main.py').read_text()
arlene = functions(arlene_source)
capacity = functions((EXP / 'research/notebooks/tetsutani/shape-the-shop-work-the-pasture-kaggriculture/extracted_MAIN_B64.py').read_text())
identical = [name for name, value in capacity.items() if arlene.get(name) == value]
assert len(identical) == 7
now = datetime.now(timezone.utc).isoformat()
audit = {
    'created_utc': now, 'inspection': 'Static AST/source audit; notebook cells not executed.',
    'router': {'source': 'https://www.kaggle.com/code/y3uanm/kaggriculture-conservative-market-router-v5',
        'all_original_functions_ast_identical': True,
        'rank_sell_orders_call_count': calls.count('_rank_sell_orders'),
        'decision': 'Existing router behavior; added price/rank helper is never called. No duplicate C++ policy.'},
    'arlene': {'source': 'https://www.kaggle.com/code/lynnsakurai/farming-score-v4-a-better-shop',
        'decoded_sha256': hashlib.sha256(arlene_source.encode()).hexdigest(),
        'unchanged_capacity_functions': identical,
        'active_changes': ['72-turn block cash budget with input reserves and sales first on shortage.',
                           'Retain zero SELL slots and credit earlier BUY_PRODUCT in sale clamp.',
                           'Final-step projected-stock liquidation, already available in the local terminal-router port.'],
        'decision': 'Port the active combination and compare source actions before league screening. Budget concept overlaps Ahmed V23; exact implementation and base route differ.'},
    'files_sha256': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in audit_dir.rglob('*') if p.is_file() and p.name != 'AUDIT.json'},
}
(audit_dir / 'AUDIT.json').write_text(json.dumps(audit, indent=2) + '\n')
means = json.loads((LATEST / 'invariant_means.json').read_text())['means']
rows = json.loads((RESEARCH / 'review_73_comparison.json').read_text())
for row in rows:
    key = {'SELL_weighted_hour': 'SELL_mean_hour', 'BUY_PRODUCT_weighted_hour': 'BUY_PRODUCT_mean_hour'}.get(row['metric'], row['metric'])
    row['global72'] = means[key]
assert len(rows) == 82
(RESEARCH / 'review_76_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
text = f'''# Review76 — {now}

Due03:08; written after the requested lineage update and collection of the live
route experiment. The previous user-facing turn completed a reproducible nine-
version lineage report from authoritative promotion results. This is progress
on reporting and attribution, not a new competitive improvement. No blocker.
Current accepted best remains observed_sale_lead_start_216. Goal remains active
through Sep10 00:47 UTC; no Git, catalog copy or official submission.

The physical/market compiler split preserves192 full game records and passes64
operational games. New route actions can now be projected before market orders
are chosen. This is required for a correct general daily compiler.

Eight crop-free herd-route policies completed256 discovery games. First results
are small and mixed: p355 cash+75.375 versus public, -32.625 versus current. The
component currently activates only after day25 with no live crops; it cannot
repair the demonstrated dense mixed-day feed losses on day18. Inspect coverage
and input delivery before extending it. Static review also identifies a possible
partial-wheat return loop; establish an actual witness before claiming its cost.

Fresh03:08 evidence contains72 player-games. Two changed notebooks were pulled
and audited. Router V5 has identical old function ASTs and an unused sale-ranking
helper: no new policy. Arlene V4 retains the known four route tapes and seven
capacity-router functions, but adds an active72-turn budget guard and sequential
sale clamp. Its final settlement already exists in our terminal port. Convert
the changed combination, verify source actions, then test broad results and
individual components. Notebook rating/validation statements remain author
claims until locally reproduced.

Priorities: continue joint route/input assignment toward mixed crop/animal days;
keep large dated composition search and feasibility errors in scope; evaluate
new public components against the strongest incumbent. Cold prototypes still
lose heavily, so their improvements alone do not satisfy the goal.

All82 global metrics use the03:08 cohort; local256 remain the frozen accepted
agent's native panel fromReview70. These unmatched cohorts do not estimate
head-to-head leaderboard strength. Final900000 remains unused.
Next review03:28 UTC; next replay refresh04:08.

| Metric | Global72 (03:08) | Current native256 |
| --- | ---: | ---: |
'''
text += ''.join(f"| {r['metric']} | {r['global72']:.4f} | {r['local256']:.4f} |\n" for r in rows)
(RESEARCH / 'review_76.md').write_text(text)
entry = f'\n{now}: Review76: split compiler192 exact records/64 operational games; herd routes256 games mixed and late-only, coverage diagnosis next. Fresh72 global games update all82 metrics. Router V5 helper unused; Arlene V4 active budget/clamp combination identified for source-parity C++ comparison. Strongest unchanged. See research/review_76.md.\n'
for name in ['IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'PROGRESS.md']:
    with (EXP / name).open('a') as out:
        out.write(entry)
print('Review76 and static notebook audit complete.')
