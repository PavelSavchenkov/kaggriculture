"""Summarize economic screens without converting them into playing-strength claims."""
from collections import Counter
from pathlib import Path
import csv
import json

RUN = Path(__file__).resolve().parent
folders = [RUN/'estimates_1000',RUN/'estimates_1002']
counts = Counter()
best = {}
seconds = 0.
controls = []
for folder in folders:
    status = json.loads((folder/'STATUS.json').read_text())
    execution = json.loads((RUN/(folder.name+'_EXECUTION.json')).read_text())
    assert execution['returncode']==0 and execution['sources_unchanged']
    controls.extend(json.loads(line) for line in (folder/'CONTROLS.jsonl').read_text().splitlines())
    rows = 0
    with (folder/'proposals.csv').open() as file:
        for row in csv.DictReader(file):
            rows += 1
            for key in ['seed','id','count','entry_day','animal_cost','seeds_saved','field_operations_delta','source_visit_deficit']:
                row[key]=int(row[key])
            for key in ['mean_own_gain','mean_margin_gain','margin_sd','risk_score','estimate_seconds']:
                row[key]=float(row[key])
            seconds += row['estimate_seconds']
            counts[f"count_{row['count']}"] += 1
            counts[f"positive_count_{row['count']}"] += row['risk_score']>0
            key = row['seed'],row['count']
            if key not in best or (row['risk_score'],-row['source_visit_deficit']) > (best[key]['risk_score'],-best[key]['source_visit_deficit']):
                best[key] = row
    assert rows == status['proposals']
assert len({r['seed'] for r in controls}) == len(controls)
total = sum(counts[f'count_{n}'] for n in [1,2,3])
report = {'controls':controls,'unique_control_seasons':len(controls),'full_control_games':len(controls)*2,
    'proposals':total,'forecast_scenarios_per_proposal':32,'estimate_seconds':seconds,
    'mean_microseconds_per_proposal':seconds*1e6/total,'counts':dict(counts),
    'best_by_seed_and_count':list(best.values()),
    'limitations':['Retrospective source calendars can contain future branch decisions.',
        'Labor cost omitted pending exact compilation; source visit deficit does not prove extra hires necessary.',
        'Single public-router opponent and seat0; exposed search seeds, not an independent playing-strength audit.',
        'Daily estimator misses intraday financing, storage and rival future expansion.']}
(RUN/'ESTIMATION_ANALYSIS.json').write_text(json.dumps(report,indent=2)+'\n')
lines = ['# Fixed-calendar group screen','',
    f'{total:,} proposals across {len(controls)} shop scenarios; {len(controls)*2} full current-agent/control games. ',
    'Both complete action hashes, money, production, sales and discards match in every control. ',
    'All extracted crop lifetimes reproduce their exact harvested output.','',
    f'{seconds:.3f} seconds inside the estimator; {seconds*1e6/total:.2f} microseconds per proposal including32 shop samples. ',
    'This excludes full-game source extraction, process startup and file output.','',
    '| Animals added | Proposals | Positive score before labor |','| --- | ---: | ---: |']
for n in [1,2,3]:
    lines.append(f"| {n} | {counts[f'count_{n}']:,} | {counts[f'positive_count_{n}']:,} |")
lines += ['', 'Best estimated groups vary with the scenario. These are search candidates, not ',
    'validated recommendations. Full-season compiler results and a legal runtime branch ',
    'selector are still required before any league claim. See ESTIMATION_ANALYSIS.json ',
    'for every selected per-scenario candidate and the explicit model limitations.','']
(RUN/'ESTIMATION_RESULTS.md').write_text('\n'.join(lines))
print({k:report[k] for k in ['proposals','full_control_games','estimate_seconds','mean_microseconds_per_proposal']})
print('Best three-animal species:',dict(Counter(tuple(p.split(':')[2] for p in r['rotations'].split(';')) for r in best.values() if r['count']==3)))
