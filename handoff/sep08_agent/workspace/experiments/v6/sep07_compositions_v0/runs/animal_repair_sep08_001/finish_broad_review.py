"""Record every matchup tradeoff and combine the completed guard investigation."""
from pathlib import Path
import csv
import hashlib
import json

RUN = Path(__file__).resolve().parent
report_path = RUN / 'BROAD_RESULTS.json'
report = json.loads(report_path.read_text())
guard = json.loads((RUN / 'GUARD_REVIEW.json').read_text())
assert guard['all_misses_traced'] and guard['passes_declared_guard_investigation_gate']
assert report['games'] == guard['games_in_broad_audit'] == 132864
rows = []
for row in report['rows']:
    a, b = row['candidate_metrics'], row['parent_metrics']
    item = {key: row[key] for key in ('panel', 'candidate', 'parent', 'opponent')}
    item.update(games_per_agent=a['games'], candidate_win_score=a['utility'], parent_win_score=b['utility'])
    for metric in ('utility', 'cash', 'margin', 'cash_cvar10', 'margin_cvar10'):
        item['gain_' + metric] = a[metric] - b[metric]
    for metric in ('faults', 'hire_cost'):
        item['gain_' + metric] = row['gain_' + metric]
    rows.append(item)
assert len(rows) == 159
with (RUN / 'BROAD_TRADEOFFS.csv').open('w') as stream:
    writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
eligibility = []
for item in report['eligibility']:
    gates = dict(item['gates'], no_uninvestigated_guard_misses=True)
    eligibility.append({'candidate': item['candidate'], 'gates': gates,
        'eligible_after_investigation': all(gates.values()),
        'failed': [name for name, passed in gates.items() if not passed]})
assert not any(item['eligible_after_investigation'] for item in eligibility)
summary = {'games': report['games'], 'matchup_comparisons': len(rows),
    'report_sha256': hashlib.sha256(report_path.read_bytes()).hexdigest(),
    'tradeoffs_sha256': hashlib.sha256((RUN / 'BROAD_TRADEOFFS.csv').read_bytes()).hexdigest(),
    'guard_investigation_complete': True, 'eligibility': eligibility,
    'decision': 'No research promotion. The submitted artifact remains operational; numeric promotion gates fail.',
    'scope': 'All candidate-parent and candidate-candidate comparisons retained. Tail differences compare each policy worst-decile distribution; they are not means on a common selected subset.'}
(RUN / 'FINAL_BROAD_REVIEW.json').write_text(json.dumps(summary, indent=2) + '\n')
body = '''# Completed broad-result review

All 132,864 games are complete. Every missed guard is now classified: 180 games
have the known tile38 wheat divergence, and two native PASS games have an unused
empty-coop build blocked by a tile73 weed. All checked animal states still match.
The gate requires investigation, not zero guard misses. This completes that
investigation; it does not erase wheat loss, failed actions, or numeric failures.

Neither candidate passes the registered numeric gates. No research promotion.
The uploaded q24 agent remains submitted and operational. Its large win gain
includes a substantial Yusuke counter and a King regression. A positive mean
native margin gain with a lower confidence bound of -$5.89 remains inconclusive
under the declared positive-lower-bound rule.

The table below preserves every panel/opponent comparison for all three pairs.
Win gains are percentage points. Cash and margin are in-game dollars. The tail
columns compare the worst 10% of each distribution separately. CSV retains
sample sizes, both win scores, fault changes and hire-cost changes as well.
Different panels and opponent groups must not be pooled as interchangeable.
'''
pairs = list(dict.fromkeys((row['candidate'], row['parent']) for row in rows))
for candidate, parent in pairs:
    body += f'\n## {candidate} versus {parent}\n\n'
    body += '| Panel / opponent | Win gain pp | Own cash | Margin | Cash tail | Margin tail |\n| --- | ---: | ---: | ---: | ---: | ---: |\n'
    selected = [row for row in rows if (row['candidate'], row['parent']) == (candidate, parent)]
    for row in selected:
        body += f"| {row['panel']} / {row['opponent']} | {100*row['gain_utility']:+.3f} | {row['gain_cash']:+.2f} | {row['gain_margin']:+.2f} | {row['gain_cash_cvar10']:+.2f} | {row['gain_margin_cvar10']:+.2f} |\n"
    worst = sorted(selected, key=lambda row: row['gain_margin'])[:4]
    print(candidate, 'versus', parent, 'largest mean-margin regressions:',
        [(row['panel'], row['opponent'], round(row['gain_margin'], 2), round(row['gain_margin_cvar10'], 2)) for row in worst])
body += '\nSources: BROAD_RESULTS.json, GUARD_REVIEW.json and the frozen broad_2360000 protocol/results.\n'
(RUN / 'FINAL_BROAD_REVIEW.md').write_text(body)
print(json.dumps({'eligibility': eligibility, 'comparisons': len(rows)}, indent=2))
