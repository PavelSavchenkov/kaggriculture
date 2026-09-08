from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import statistics

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
rows = []
controls = 0
for case in ['mixed', 'goose', 'p355', 'p362', 'p4', 'p55']:
    for opponent in ['public_router', 'observed_sale_lead_start_216']:
        data = [json.loads((RUN / f'discovery/joint_routes_{case}_m{mode}_vs_{opponent}.json').read_text()) for mode in range(3)]
        old = json.loads((EXP / f'runs/compiler_split_sep08_001/discovery/compiler_split_{case}_vs_{opponent}.json').read_text())
        assert data[0]['games'] == old['games']
        controls += len(old['games'])
        modes = []
        for mode, d in enumerate(data):
            mean = lambda f: statistics.mean(f(g) for g in d['games'])
            lives = [life for g in d['games'] for life in g['profile']['lives'] if life[0] >= 9]
            active = sum(life[6].bit_count() for life in lives)
            modes.append({'mode': mode, 'cash': mean(lambda g: g['cash']),
                'margin': mean(lambda g: g['cash'] - g['opponent_cash']),
                'hires': mean(lambda g: g['profile']['hires']), 'hire_cost': mean(lambda g: g['profile']['hire_cost']),
                'moves': mean(lambda g: sum(g['profile']['successful'][1:5])), 'faults': mean(lambda g: g['unit_faults']),
                'feed_fraction': sum(life[8].bit_count() for life in lives) / active,
                'care_fraction': sum(life[9].bit_count() for life in lives) / active,
                'produced': [mean(lambda g, i=i: g['produced'][i]) for i in range(9)]})
        for mode in modes:
            mode['cash_gain'] = mode['cash'] - modes[0]['cash']
            mode['margin_gain'] = mode['margin'] - modes[0]['margin']
            mode['produced_gain'] = [a - b for a, b in zip(mode['produced'], modes[0]['produced'])]
        rows.append({'case': case, 'opponent': opponent, 'modes': modes})
report = {'completed_utc': datetime.now(timezone.utc).isoformat(), 'games': 576,
    'split_control_full_records_equal': controls, 'rows': rows,
    'decision': 'Reject this joint-route replacement as a general compiler improvement. Small mixed farm improves slightly, but dense and goose farms lose. Inspect blocked input tasks and dependent crop work before another route rule.',
    'causal_scope': 'Mode1 starts day14 and retains earlier behavior. On p355 versus public, eggs+11 and wool+13.75 accompany strawberries-49.25 and milk-13.875; hires unchanged. More animal service can displace crop work. These aggregate changes do not yet prove the specific internal scheduling cause.',
    'next': 'Trace the first changed mixed day and count unfinished/blocked tasks, current inputs and idle workers. Compare with a day-solver feasible schedule on exactly that start state. Incorporate dependent task work and availability into route assignment.'}
(RUN / 'ANALYSIS.json').write_text(json.dumps(report, indent=2) + '\n')
paths = [RUN / 'compiler/source/agent.hpp', RUN / 'compiler/source/agent.cpp', RUN / 'routes.hpp', RUN / 'prepare.py']
(RUN / 'SOURCE_HASHES.json').write_text(json.dumps({str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}, indent=2) + '\n')
text = '# Joint crop/animal routes\n\n' + report['decision'] + '\n\n'
text += f'576 full games; {controls} complete old compiler controls exact. No strongest-agent promotion.\n\n'
text += '| Farm | Opponent | Start day | Cash gain | Margin gain |\n| --- | --- | ---: | ---: | ---: |\n'
for row in rows:
    for mode in row['modes'][1:]:
        text += f"| {row['case']} | {row['opponent']} | {14 if mode['mode'] == 1 else 0} | {mode['cash_gain']:+.2f} | {mode['margin_gain']:+.2f} |\n"
text += '\n' + report['causal_scope'] + '\n\n' + report['next'] + '\n'
(RUN / 'RESULTS.md').write_text(text)
(RUN / 'README.md').write_text('# Joint day-task compiler experiment\n\nRead RESULTS.md, ANALYSIS.json and LINEAGE.json. prepare.py copies the unchanged split compiler and exposes its existing tile tasks, then creates18 route/control policies. run_discovery.py builds and runs the576-game comparison; analyze.py requires192 exact old full records. Market orders are projected from the new physical actions. Cold compositions and attributed source lives are preserved. General routing replacement rejected; diagnostic work remains.\n')
print('576 full games;', controls, 'exact old compiler controls.')
