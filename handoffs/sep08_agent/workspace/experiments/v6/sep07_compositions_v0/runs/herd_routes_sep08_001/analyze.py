from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
rows = []
controls = 0
for case in ['mixed', 'goose', 'p355', 'p362']:
    for opponent in ['public_router', 'observed_sale_lead_start_216']:
        data = [json.loads((RUN / f'discovery/herd_routes_{case}_m{mode}_vs_{opponent}.json').read_text()) for mode in [0, 1]]
        old = json.loads((EXP / f'runs/compiler_split_sep08_001/discovery/compiler_split_{case}_vs_{opponent}.json').read_text())
        assert data[0]['games'] == old['games']
        controls += len(old['games'])
        modes = []
        for mode, d in enumerate(data):
            games = d['games']
            mean = lambda f: statistics.mean(f(g) for g in games)
            animals = [life for g in games for life in g['profile']['lives'] if life[0] >= 9]
            active = sum(life[6].bit_count() for life in animals)
            modes.append({'mode': mode, 'games': len(games), 'cash': mean(lambda g: g['cash']),
                'margin': mean(lambda g: g['cash'] - g['opponent_cash']),
                'hires': mean(lambda g: g['profile']['hires']),
                'moves': mean(lambda g: sum(g['profile']['successful'][1:5])),
                'fed_fraction': sum(life[8].bit_count() for life in animals) / active,
                'care_fraction': sum(life[9].bit_count() for life in animals) / active,
                'produced': [mean(lambda g, i=i: g['produced'][i]) for i in range(9)]})
        for row in modes:
            row['cash_gain'] = row['cash'] - modes[0]['cash']
            row['margin_gain'] = row['margin'] - modes[0]['margin']
        rows.append({'case': case, 'opponent': opponent, 'modes': modes,
                     'unchanged_records': sum(a == b for a, b in zip(data[0]['games'], data[1]['games']))})
report = {'created_utc': datetime.now(timezone.utc).isoformat(), 'games': 256,
    'old_control_full_records_equal': controls, 'rows': rows,
    'decision': 'Mixed small late-game changes. No broad strength improvement; measure activation and actual route input behavior before expanding to mixed days.'}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
hashes = {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
          for p in [RUN / 'routes.hpp', RUN / 'prepare.py', EXP / 'runs/compiler_split_sep08_001/compiler/source/agent.cpp']}
(RUN / 'SOURCE_HASHES.json').write_text(json.dumps(hashes, indent=2) + '\n')
(RUN / 'BUILD_AMENDMENT.json').write_text(json.dumps({
    'failure': 'std::min received narrow Count and int; use explicit std::min<int>.',
    'failed_log': 'build_failed_count_type.log', 'lineage_hashes_before_build': 'LINEAGE.json',
    'validated_source_hashes': 'SOURCE_HASHES.json', 'scope': 'Type correction before successful build; no completed candidate result predates it.'}, indent=2) + '\n')
text = '# Crop-free herd routes\n\n' + report['decision'] + '\n\n'
text += f'256 full discovery games; {controls} old controls match exactly.\n\n'
text += '| Farm | Opponent | Cash gain | Margin gain | Unchanged games |\n| --- | --- | ---: | ---: | ---: |\n'
for row in rows:
    mode = row['modes'][1]
    text += f"| {row['case']} | {row['opponent']} | {mode['cash_gain']:+.2f} | {mode['margin_gain']:+.2f} | {row['unchanged_records']}/16 |\n"
(RUN / 'RESULTS.md').write_text(text)
print(controls, 'exact controls; 256 games analyzed.')
for row in rows:
    print(row['case'], row['opponent'], row['modes'][1]['cash_gain'], row['unchanged_records'])
