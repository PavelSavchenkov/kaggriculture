"""Review deployment validation and measured care/workforce results."""
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
EXP = HERE.parent
old = (HERE / 'review_107.md').read_text()
table = old[old.index('| Metric |'):]
assert sum(line.startswith('| ') for line in table.splitlines()) == 84
stamp = datetime.now(timezone.utc).isoformat()
body = f'''# Review 108 — {stamp}

One submission remains authorized and has not been uploaded. Source is frozen in
submissions/sep8-composition-adaptive-v1. The provisional q24/animal/premium
candidate now has a standalone Python archive generated from frozen C++ tables.
Every inherited table matches the last submitted adapter's data. The initial
eight official full games match all C++ actions; the larger 132-game package
audit is running. Frozen C++ native tests cover 4,096 games against the teammate
and 4,096 against the last submission. Results remain pending. The first adapter
parity failure caught the opening's 13-wheat reserve and next-turn sale adjustment;
the compiler was corrected, preserving the C++ agent. Keep the failed evidence.

The 132,864-game independent league audit is still running. Some activated animal
courses miss strict day guards in new worlds. An offline observation inspector
is being compiled to list every differing field and reproduce the complete game
hash, cash, output and faults. Do not accept the candidate while those failures
are unexplained. Promotion criteria remain those registered before this audit;
King's tradeoff must remain visible. Accepted reference is empty_sale_slots_m2.

The separate day-solver experiment has made a measured improvement. Independent
full-game checks of three completed courses pass every day endpoint and preserve
both farms' output. Omitting care whose bonus would hit the cow's storage cap
alone saves no money in either context. Allowing one fewer inherited worker in
the search saves three hires and $178 in the seed1014 course: own cash131290 to
131468, rival cash123894 unchanged, hire cost5044 to4866. This is one fixed world,
not a deployable policy gain. The second context remains in compilation. It
supports jointly optimizing service exceptions and workforce, rather than
assuming a shorter task list will automatically reduce paid labor.

The user's original ideas remain in scope: dated farm composition, fast economic
ranking, exact schedules for promising farms, observed-shop branching, opponent
responses, replay borrowing with lineage, larger and mixed farms, cold starts,
and a growing league. Current priority is the explicitly requested submission.
Afterward, incorporate validated workforce savings and repair mixed-course
funding before expanding the composition search. Keep the observation-only rival
flow forecast and Tetsutani sale-merging component queued. No GPU job is needed.

The 82 comparisons below reuse the latest 14:08 global cohort (72 player records,
64 unique replays) and the accepted agent's native256 baseline. These unmatched
cohorts describe behavior; they do not measure a causal advantage. Final900000 is
unused. Next review15:10UTC; next replay and notebook refresh15:08UTC.

'''
(HERE / 'review_108.md').write_text(body + table)
note = f'\n{stamp}: Review108. One submission in preparation, no upload yet. Frozen adapter first8 full games exactly match C++; larger packed132, C++8192, and broad132864 audits pending. Guard-miss inspector pending. Care-only saves no money; below-parent workforce search saves3 hires/$178 in independently audited seed1014 with identical production. Latest global72/64 metrics reused with localnative256. Nextreview15:10/refresh15:08.\n'
for name in ('PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md', 'LINEAGE.md'):
    with (EXP / name).open('a') as stream:
        stream.write(note)
print('Review108 written with 82 metrics.')
