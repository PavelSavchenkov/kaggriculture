from pathlib import Path
from datetime import datetime, timezone
import json

EXP = Path(__file__).resolve().parents[2]
now = datetime.now(timezone.utc).isoformat()
rows = json.loads((EXP / 'research/review_52_comparison.json').read_text())
assert len(rows) == 82
text = f'''# Review 53 — {now}

Due19:28UTC. Goal continues through September8,00:47UTC. Current reference
remains opening_q32_b13_v1; no Git, official catalog update or submission.

The day solver materially changes the crop-to-goose decision. The first
3-second schedules used9/$987 extra hires in the off continuation and8/$898
in the on continuation. Longer searches plus a terminal service adjustment
remove8/$898 in both. Off retains one extra $89 hire on day22; on uses the
unchanged crop workforce. Full64-game audits per leaf preserve every sold
quantity and rival action hash. Each activated game leaves one unsold final
fertilizer uncollected; other production stays equal. All cash improvement
equals saved hire cost. Mean margin against the compiled crop control is now
+$98.5625 off and+$161.890625 on. These remain discovery results against
public_router, not a promoted complete-policy or broad-league improvement.

Offday23 found a fixed-workforce route after93.47seconds. Offday22 remains
UNKNOWN after120seconds. Borrowing the on route fails because one strawberry
does not reach the shed in time. Delaying one sale to any hour18..23, even
adding one available PASS-to-DROP action, does not satisfy the exact contract.
This is a useful stock/timing constraint, not a proof that the hire is needed.
Do not let this last $89 block complete-policy comparisons and new proposals.

Seven repository-format WIP agents are frozen under this run: initial goose,
optimized goose, two forced-leaf diagnostics, unchanged crops, initial cow and
initial sheep. The normal policy keeps the original observed day20 berry rule.
Its exact day20 guards match. Build and package parity checks are underway.
Next compare all composition alternatives with q32 on common seeds, then use
observed demand/economic estimates to select only useful investments. Initial
cow/sheep labor costs are not minimum proofs either. The new conversion is a
second actual investment beyond the earlier adaptive animal choice.

Fresh19:02 notebook pulls are stored without executing them. Can Specialists
Beat One Agent is a presentation/plotting notebook, with no executable policy;
its performance numbers are author claims. V5 Hybrid appears to reproduce the
already ported Thomas V5 controller; exact payload comparison remains pending.
Fields of Fortune contains a reactive crop/animal ranking policy and needs a
static audit before deciding whether a C++ port adds league diversity.

Keep the full original objective: dated composition search, cheap biology,
economics and service estimates, placement and labor/trade compilation,
prediction-versus-execution feedback, all animal species and wait, large/cold
proposals, borrowed top-player behavior, and a growing opponent league. The
goose example confirms why estimated value and realized execution must feed
back into each other; a poor short solve can reject a profitable composition.

The82 comparisons below retain the latest19:02 global72 cohort and promoted
q32 local64 profile. They are unchanged since review52; no new global cohort
or promoted agent exists. Different cohorts are not paired improvements.
Next review19:48UTC; public refresh around20:02UTC.

| Metric | Global72 | q32 local64 |
| --- | ---: | ---: |
'''
for row in rows:
    text += f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP / 'research/review_53.md').write_text(text)
(EXP / 'research/review_53_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
entry = f'\n{now}: Review53: longer solver plus terminal service adjustment saves8/$898 hires per activated goose course in both leaves. Sold quantities/rival actions unchanged in64 each; one unsold fertilizer uncollected. Mean margin vs crop control+$98.56off/+$161.89on. Lastoff day22 UNKNOWN120. Seven complete WIP packages frozen, parity/league next. Global1902 remains latest; notebook static audit partly done. Next19:48.\n'
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md']:
    with (EXP / name).open('a') as output:
        output.write(entry)
print('Review53 recorded', now)
