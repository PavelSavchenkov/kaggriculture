from pathlib import Path
from datetime import datetime, timezone
import json

EXP = Path(__file__).resolve().parents[2]
now = datetime.now(timezone.utc).isoformat()
rows = json.loads((EXP / 'research/review_53_comparison.json').read_text())
assert len(rows) == 82
text = f'''# Review54 — {now}

Due19:48UTC. The original goal remains active through September8,00:47UTC.
Current reference remains opening_q32_b13_v1. No Git or new submission.

The user's day-solver suggestion worked. More exact search removes8/$898
extra hires in each goose continuation. On now has no extra hires; off still
has one $89 hire on day22. Full-game sales remain unchanged. Final-day work
leaves one unsold fertilizer uncollected. Cow/sheep received the same30-second
retry: all extra hires disappear for sheep and the on cow course, with exact
production/sales/rival-action preservation in64games per leaf. Off cow still
has three unresolved days. UNKNOWN remains a search result, not a minimum.

The first complete goose policy passed a3328-game discovery screen but failed
the43008-game fresh broad screen. Its wrapper had replaced the parent's
tomato-selected future despite matching the same physical entry state. In181
of1024 direct games it lost24tomatoes and added wheat/goose, averaging-$1005.48
margin change. The intended392 wheat-to-goose games averaged+$199.11. This
exposes a compilation-context error, not an error in the predicted28eggs.
The small crop-control loss was an early signal we should have investigated.

late_goose_wheat_context fixes this by preserving the parent's existing
day12 two-tomato-shop rule. On a second, newly preregistered43008-game panel,
it wins420, ties480 and loses124 direct parent games, mean margin+$80.35.
Every opponent's paired mean margin improves. However current grouped utility
falls.9454834 to.9407837, with95% gain[-.0076904,-.0020142]; historical and
individual utility gates also fail. No promotion. The corrected context removes
the large unintended losses; an economic investment rule is still needed.

Next select among complete goose/cow/sheep calendars and retaining crops using
the existing fast whole-farm observed-market estimator. Use their actual
compiled daily sales/buys, fixed seed/animal costs, and achievable hire costs.
Separate the already-used1790000/1810000 panels from new validation. Branch on
future berry demand only when those shops are observed, and compare conservative
or sampled expected continuation values at day13. Preserve intent as well as
physical compatibility when importing any new calendar. This is a direct
instance of the original estimate/compile/measure feedback loop.

New public notebook audit is complete: V5 Hybrid repeats the already ported
five tapes/trees and adds a small weed-clearing rule. The C++ diagnostic port
has896 discovery games, unchanged win counts, active-opponent margin gains0
to+$6.69 and PASS loss-$4.86. It is not a new strong family. Fields of Fortune
has geographical crop zones and greedy placement but explicitly disables
animals; its valuations omit important biology/service costs. Can Specialists
Beat One Agent is presentation material without an executable policy. Exact
hashes, comparison and decisions are in research/refresh_1902/NOTEBOOK_AUDIT.md.

Keep the full original goal active: dated composition search, fast economics
and service feasibility, placement/worker/trade optimization, general animal
and wait choices, larger/cold proposals, reusable top-player components, and
growing-league validation. Neither good schedules nor positive average cash
alone is enough to promote a policy that loses league win probability.

The82 comparisons below are unchanged19:02 global72 and promoted-q32 local64.
No new promoted policy or global replay cohort exists. Next review20:08UTC;
refresh public sources around20:02UTC. The final900000 seed reserve is unused.

| Metric | Global72 | q32 local64 |
| --- | ---: | ---: |
'''
for row in rows:
    text += f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP / 'research/review_54.md').write_text(text)
(EXP / 'research/review_54_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
entry = f'\n{now}: Review54: day solver saves8/$898 hires in both goose leaves; all extra sheep and on-cow hires also removed. Unconditional goose failed43008fresh games because it overwrote tomato intent. Context correction fixes that and gains+$80.35 direct on another43008fresh games, but grouped utility still fails; q32 remains reference. Next general animal/wait selector from observed-market estimates and compiled costs. Notebook1902 audits complete. Next20:08; public refresh20:02.\n'
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md']:
    with (EXP / name).open('a') as output:
        output.write(entry)
print('Review54 recorded', now)
