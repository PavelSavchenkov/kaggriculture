from pathlib import Path
from datetime import datetime, timezone
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
path = EXP / 'results/opening_market_validation.json'
report = json.loads(path.read_text())
assert all(report['gates'].values())
league = json.loads((RUN / 'FRESH_LEAGUE_VALIDATION.json').read_text())
assert league['status'] == 'All preregistered additional league gates passed'
now = datetime.now(timezone.utc).isoformat()
report['status'] = 'Promoted as experimental reference after broad, operational, native, tail and expanded opening-league review'
report['promoted_utc'] = now
report['expanded_opening_league'] = league
report['causal_review'] = {
    'finding': 'Opening quantity affects opponent first-day cash and later controller behavior. The hire-preserving control isolates most older-opponent gains at about $5.',
    'correction': league['correction'],
    'accepted_tradeoffs': 'Two fresh opponents lose one strict win each, historical group utility is nearly unchanged, and public-sixday mean improvement includes negative tail changes. All original numeric gates pass; no uniform superiority claim.',
    'tail_evidence': 'runs/opening_market_validation_001/TAIL_DIAGNOSTICS.json',
    'scope': 'Only experimental reference changes. The committed catalog package and last Kaggle submission remain their separately frozen versions.'}
path.write_text(json.dumps(report, indent=2) + '\n')
reference = {'name': report['candidate'], 'path': 'runs/opening_market_search_001/proposals/opening_q32_b13_v1',
             'promoted_utc': now, 'report': 'results/opening_market_validation.json',
             'previous': report['parent'], 'committed_catalog': 'agents/external/bohann_opening_v1',
             'last_submission': 'investment_context_guarded_001_best'}
(EXP / 'CURRENT_REFERENCE.json').write_text(json.dumps(reference, indent=2) + '\n')
agent_readme = RUN / 'proposals/opening_q32_b13_v1/README.md'
agent_readme.write_text('''# opening_q32_b13_v1

Promoted experimental reference after the 20-opponent fresh panel, required
operational checks, native checks, and an additional 19-opening fresh audit.
See `../../../../results/opening_market_validation.json` from this package.

Retains the complete adaptive crop/animal policy from crop_mix_t2_wheat. At step
0, prepend a 32-unit wheat buy/sell round trip and set the initial wheat buffer
to 13. At step 1, reduce the original wheat sale by the initial-stock reduction;
retain all other parent orders and optimized hires. Only legal observations are
used. Reset clears the original-stock adjustment.

The round-trip idea comes from Bohann Wang episode 106497007, seat 0; quantity,
stock and hiring changes come from this experiment's measured ablations. Full
inherited crop, animal, worker and replay lineage remains with the parent.

Original fresh direct Bohann result: 1,020 wins / 1,024 games, mean margin
$8,415.24. Additional independent audit: 1,023 / 1,024 against Bohann and
1,005 / 1,024 against the q81 hire control. Most older-opponent gains are about
$5; two opponents lose one strict fresh win each, and several tails decline.
This is the current experimental reference, not an uploaded or committed agent.
''')
readme = EXP / 'README.md'
text = readme.read_text()
text = text.replace('Current broad search starting point (17:20 UTC):', '''Current broad search starting point (18:43 UTC):
`runs/opening_market_search_001/proposals/opening_q32_b13_v1/`.
Read `CURRENT_REFERENCE.json` and `results/opening_market_validation.json` for
the promotion evidence, all tradeoffs, and the additional opening-variant league.
The committed catalog reference remains `agents/external/bohann_opening_v1/`.

Previous reference (17:20 UTC):''')
readme.write_text(text)
entry = f'''\n{now}: Promoted opening_q32_b13_v1 as experimental reference. Original fresh20 panel: 1020/1024 direct Bohann, +$8415.24 mean; all paired means positive, historical utility nearly unchanged, two single-win regressions retained. Additional19-opening audit: 24320 games, no losing matchup; independent Bohann1023/1024, q81control1005/1024; native256/256 and249/256. All prior operational/native gates and tail review complete. Across-opponent cash counterexample corrected. No catalog/Git/upload change. Continue larger crop-to-animal composition work.\n'''
for name in ['PROGRESS.md', 'IDEAS_LEDGER.md']:
    with (EXP / name).open('a') as file:file.write(entry)
print(reference)
