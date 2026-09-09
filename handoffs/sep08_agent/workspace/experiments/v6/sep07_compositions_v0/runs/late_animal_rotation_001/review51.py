from pathlib import Path
from datetime import datetime, timezone
import json
import statistics

EXP=Path(__file__).resolve().parents[2]
now=datetime.now(timezone.utc).isoformat()
profile_path=EXP/'runs/opening_market_validation_001/profiles/opening_q32_b13_v1_vs_public_router.json'
data=json.loads(profile_path.read_text())
means=json.loads(profile_path.with_suffix('.profile.json').read_text())['means']
old_rows=json.loads((EXP/'research/review_50_comparison.json').read_text())
rows=[]
for old in old_rows:
    name=old['metric']; key=name.replace('harvest_','produced_') if name.startswith('harvest_') else 'faults' if name=='unit_faults' else name
    if name.endswith('_weighted_hour'):
        field='sell_hours' if name.startswith('SELL') else 'buy_hours'
        numerator=denominator=0
        for game in data['games']:
            for hour,quantities in enumerate(game['profile'][field]):
                numerator+=hour*sum(quantities);denominator+=sum(quantities)
        local=numerator/denominator
    else:
        assert key in means,key
        local=means[key]
    rows.append({'metric':name,'global72':old['global72'],'local64':local})
assert len(rows)==82
estimates=[]
import csv
for leaf in ['off','on']:
    proposals=list(csv.DictReader((EXP/f'runs/late_animal_rotation_001/estimate_{leaf}_1000_v2/proposals.csv').open()))
    chosen=[r for r in proposals if r['cell']=='31' and r['entry_day']=='13' and r['animal']=='9'][0]
    estimates.append({'leaf':leaf,**chosen})
text=f'''# Review 51 — {now}

Due 18:48 UTC. Full goal remains active through September 8, 00:47 UTC.
Previous turn verified completed user work; this turn adds opening-league
evidence, a promoted experimental reference, and an implemented larger
composition estimator/compiler. No Git or external submission is authorized.

Current reference is opening_q32_b13_v1. Its original 20-opponent fresh panel
passes all preregistered numeric and operational checks. The additional opening
league adds 24,320 fresh games: independent direct Bohann 1023/1024 and q81
hire-control 1005/1024; native 256/256 and 249/256. No opening matchup loses
on utility; q0 buffer13 is level. The earlier supposed control counterexample
was an invalid comparison across opponents and is corrected in the report.
The committed catalog remains Bohann, and the last upload remains investment.

Do not overstate the improvement: the large gain is against the Bohann parent,
most older-opponent gains are about $5, old-group utility is nearly unchanged,
two opponents lose one fresh win each, and several tails fall. The frozen broad
report retains all these tradeoffs. New public-router profiles report mean cash
${means['cash']:.2f}, {means['hires']:.4f} hires and ${means['hire_cost']:.4f} hire cost.

The crop-to-animal run is now concrete. The dated C++ screen generates 198
natural crop-release × species proposals per source continuation and verifies
all 239 source crop lives. It includes goose, cow and sheep; retaining crops is
the baseline. Observed-state valuation subtracts lost future crops and includes
animal purchase, feed, fertilizer and whole-herd prices. Source-quote valuation
is separately labeled offline. Labor and financial feasibility still require
exact compilation. Cached source visits keep candidate evaluation inexpensive;
the first distance estimate used the wrong shed coordinates and was corrected.

The initial selected opportunity is a day-13 goose on cell31. Day-17 cell22
looks negative even before labor. Both complete goose continuations compile
through day28 with exact physical/stock endpoints, preserving either later
berry continuation. Several days need an additional hire. The first terminal
day attempt could not insert an extra sale into a full market action. Version2
reuses empty slots or combines the same product sale, and is running goose
and unchanged-crop control courses for both leaves. No animal candidate is
promoted. Final-day contracts use an offline PASS padding hour only; real games
still stop at hour22, and missing required work must fail the endpoint check.

The source-contract test is necessary because rebuilding days can change labor,
discards and trading even without changing composition. Measure those changes
before attributing any gain to the new goose. After complete courses work,
compare estimates and exact games across observed shop contexts, then preserve
both future berry branches in an executable observation-only candidate.

Original objective remains the guide: dated composition search, cheap economics
and service estimates, better placement, exact schedules and trade timing,
estimator-versus-execution feedback, general animal/wait decisions, fresh
replay learning, and a growing league. Multi-investment construction is now
being exercised but is not yet a profitable general policy; cold independent
construction also remains unfinished. Next review 19:08 UTC. Refresh the public
cohort around18:59; current global evidence is still the17:59 cohort.

The 82 metrics below compare different global/local scenarios and are diagnostic,
not paired superiority claims. Local values use the actual newly promoted agent.

| Metric | Global72 | q32 local64 |
| --- | ---: | ---: |
'''
for row in rows:text+=f"| {row['metric']} | {row['global72']:.4f} | {row['local64']:.4f} |\n"
(EXP/'research/review_51.md').write_text(text)
(EXP/'research/review_51_comparison.json').write_text(json.dumps(rows,indent=2)+'\n')
(EXP/'runs/late_animal_rotation_001/SELECTED_ESTIMATES.json').write_text(json.dumps(estimates,indent=2)+'\n')
entry=f'\n{now}: Review51 records all82 metrics for new q32 reference. Larger198-proposal crop-to-animal screen and both full suffix compilers implemented. First goose schedules exact through28; terminal market-slot correction and unchanged-crop controls running. No animal promotion. Next19:08; public refresh18:59; goal through00:47.\n'
for name in ['PROGRESS.md','IDEAS_LEDGER.md']:
    with (EXP/name).open('a') as file:file.write(entry)
print('Review51',now,'local cash',means['cash'],'hires',means['hires'],'cost',means['hire_cost'])
print(estimates)
