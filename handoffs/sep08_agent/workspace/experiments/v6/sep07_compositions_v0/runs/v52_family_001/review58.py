from pathlib import Path
from datetime import datetime, timezone
import csv
import json

EXP = Path(__file__).resolve().parents[2]
now = datetime.now(timezone.utc).isoformat()
previous = json.loads((EXP / 'research/review_57_comparison.json').read_text())
global_data = json.loads((EXP / 'research/refresh_2102/invariant_means.json').read_text())['means']
trades = list(csv.DictReader((EXP / 'research/refresh_2102/transactions.csv').open()))
rows = []
for old in previous:
    metric = old['metric']
    if metric.endswith('_weighted_hour'):
        operation = metric.removesuffix('_weighted_hour')
        selected = [r for r in trades if r['operation'] == operation]
        value = sum(float(r['hour']) * float(r['actual']) for r in selected) / sum(float(r['actual']) for r in selected)
    else:
        value = global_data[metric]
    rows.append({**old, 'global72': value})
assert len(rows) == 82
text = f'''# Review58 — {now}

Due21:08UTC. Current reference stays late_value_s32_t0_r05. Previous turn made
progress on the user-requested PDF; this turn adds an independent larger farm
transfer, exact day compilation and full-game causal evidence. Goal remains
active through00:47UTC, with no Git or new submission.

All256 instrumented V5/2 versus portfolio records match the previous generic
arena exactly. Both farms have equal physical states atstep1;254/256 still
have identical tiles atday6. The usual differences are four wheat, one excess
fertilizer, nine wheat seeds, three carrot seeds and four strawberry seeds.
The donor's own wool route wins43/58 selected games versus our incumbent in
this discovery cohort. Source selection is not a causal animal ablation.

New full-family policies retain our opening and restore missing stocks atday6.
Wheat and cheap seeds are restored in the existing hour0 slots; strawberry
stock is restored athour1 after fertilizer sales and hires to preserve funding.
The donor's entire remaining composition replaces our later branches after
entry. Physical guards and route state remain observation-only. Four packages
and six opponents produce3840 discovery games. Wool demand>=2 beats the
parent22W94T12L/128,+$98.20, but loses mean margin against several others.
Blind wool and transfer of every donor route are much weaker. A two-Yarn gate
never activates on this small cohort; its exact baseline equality is no gain.

Day-solver V30 rebuilt17 of22 eligible source days with one fewer hire and
strict source endpoint/cash checks, totalconditional$1275. Day28 overflow was
skipped, and five days exhausted5second budgets. Packages have historical
trial suffixh18, but the actual library contains17 checked days. Runtime entry
cannot use the source day6 guard before stock restoration; later matches are
checked individually and require active donor route1.

Another1536 full discovery games test two optimized transfers. Wool2 variant
now adds$335.46 direct mean margin, still22W94T12L/128. Versus newV5/2 it wins
117/128, versusoldV5 105/128, King128/128, teammate125/128, publicrouter107/128.
The corresponding baseline has102,108,126,124,109 wins. No broad promotion.

Causal comparison of128 direct games has20 changed cases:13 save16hires/$1254
and3 save15/$1165 with identical output/sales/rivalcash. Two cases save6hires
and fund a missed sheep, adding22wool,16fertilizer and6berries, owncash+$4688.
Two save7hires/$466 and change wheat sales by3; owncash+$471,rivalcash+$6.
This is a measured cash-to-investment cascade, not uniformly pure labor.

Next run baseline and forced optimized wool family on identical observed
prefixes with entry features and actual outcomes. Estimate the value of this
larger continuation across observed shop/opponent contexts, then validate a
causal executable selector. Keep the main four-way species selector and cold
construction goal intact; this sheep family is an additional available course.

Fresh21:02 cohort:72 player-games, no changed public notebooks. Full82 metrics
below compare the new global cohort with the unchanged actual-reference64
profile; different cohorts cannot establish a paired leaderboard gap. Global
cash is${global_data['cash']:.2f}; changing prices and opponent mix also matter.
Next review21:28UTC, refresh22:02UTC. Final900000 stillunused. The original
wording and broad scope were checked in the preceding review and assessment.

| Metric | Global72 (21:02) | Portfolio local64 |
| --- | ---: | ---: |
'''
for row in rows:
    text += f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP / 'research/review_58.md').write_text(text)
(EXP / 'research/review_58_comparison.json').write_text(json.dumps(rows, indent=2) + '\n')
entry = f'\n{now}: Review58: full82 global2102 metrics, larger V5/2 family transfer and17 checked day plans. Direct+$335.46 but mixed broad results, no promotion. Causal labor savings and restored sheep purchase distinguished. Baseline/forced-family observed-prefix valuation is next. Next review21:28, refresh22:02.\n'
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md', 'PROFILING_LEDGER.md']:
    with (EXP / name).open('a') as output:
        output.write(entry)
print('Review58 recorded', now)
