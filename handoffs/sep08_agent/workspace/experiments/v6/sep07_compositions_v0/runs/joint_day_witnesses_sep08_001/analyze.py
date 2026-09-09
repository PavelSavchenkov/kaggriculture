from pathlib import Path
import json
import statistics

RUN = Path(__file__).resolve().parent
report = json.loads((RUN / 'RESULTS.json').read_text())
OPS = ['pass', 'north', 'south', 'east', 'west', 'pickup', 'drop', 'place', 'plant',
       'water', 'harvest', 'fertilize', 'dig', 'build_coop', 'build_pasture', 'feed', 'collect', 'care']


def profile(path):
    positions = [(4, 4)]
    counts = [0] * 18
    tiles = {}
    workers = {}
    lines = path.read_text().splitlines()
    assert len(lines) == 24
    for hour, line in enumerate(lines):
        values = list(map(int, line.split()))
        units, orders = values[:2]
        assert len(values) == 2+3*(units+orders)
        assert units == len(positions), (path, hour, units, positions)
        for u in range(units):
            op, arg, n = values[2+3*u:5+3*u]
            counts[op] += 1
            workers.setdefault(u, [0]*18)[op] += 1
            x, y = positions[u]
            if op >= 8 or (op == 7 and arg >= 9):
                tiles.setdefault(y*10+x, []).append({'hour': hour, 'worker': u, 'op': OPS[op]})
            if op == 1:
                y -= 1
            elif op == 2:
                y += 1
            elif op == 3:
                x += 1
            elif op == 4:
                x -= 1
            assert 0 <= x < 10 and 0 <= y < 10
            positions[u] = x, y
        for i in range(orders):
            op, arg, n = values[2+3*(units+i):5+3*(units+i)]
            if op == 1:
                spawn = min([(4, 4), (5, 4), (4, 5), (5, 5)], key=lambda p: positions.count(p))
                positions.append(spawn)
    return {'moves': sum(counts[1:5]), 'pass': counts[0], 'pickup': counts[5],
            'deposit': counts[6]+counts[7]-sum(e['op'] == 'place' for events in tiles.values() for e in events),
            'field_actions': sum(len(events) for events in tiles.values()),
            'tiles_with_multiple_workers': sum(len({e['worker'] for e in events}) > 1 for events in tiles.values()),
            'worked_tiles': len(tiles), 'counts': dict(zip(OPS, counts)), 'workers': workers, 'tile_events': tiles}


rows = []
for agent in ['joint_routes_p355_m0', 'joint_routes_p355_m1', 'joint_routes_p362_m0', 'joint_routes_p362_m1']:
    for day in [14, 15, 16]:
        folder = RUN / 'compiled' / agent
        source = profile(folder / f'day{day}_augment0_remove0/source_schedule.txt')
        variants = {}
        for augment in [0, 1]:
            result = next(r for r in report['cases'] if r['agent'] == agent and r['day'] == day and r['augment'] == augment and r['remove_hires'] == 2)
            if result['solved']:
                assert result['full_endpoint_equal'] and result['cash_equal']
                variants[augment] = {**profile(folder / f'day{day}_augment{augment}_remove2/combined_schedule.txt'),
                                     'extra_actions': {OPS[i]: n for i, n in enumerate(result['added_actions']) if n},
                                     'extra_output': result['added_produced'], 'hire_saving': result['hire_saving'],
                                     'solve_seconds': result['seconds']}
        rows.append({'agent': agent, 'day': day, 'source': source, 'two_fewer_workers': variants})
result = {'cases': len(report['cases']), 'solved': report['solved'],
          'all_augmented_two_fewer_solved': all(1 in row['two_fewer_workers'] for row in rows),
          'median_solved_seconds': statistics.median(r['seconds'] for r in report['cases'] if r['solved']),
          'rows': rows,
          'scope': 'Position and task-assignment counts from schedules already verified in the full engine. Source schedules contain sanitized accepted actions. Multiple workers on a tile establish cooperation in the solution, not that cooperation alone caused the gain.'}
assert result['all_augmented_two_fewer_solved']
(RUN / 'ANALYSIS.json').write_text(json.dumps(result, indent=2) + '\n')
text = '# Exact day scheduling results\n\n'
text += f"45/48 cases solved within a2-second allowance; every returned schedule passed full-engine checks. Median solved time {result['median_solved_seconds']:.3f}s. All12 augmented cases with two fewer workers solved.\n\n"
text += '| Farm/controller | Day | Source moves | Solved moves, more work/two fewer workers | Extra field actions | Hire saving | Tiles served by multiple workers, source → solved |\n| --- | ---: | ---: | ---: | ---: | ---: | ---: |\n'
for row in rows:
    old, new = row['source'], row['two_fewer_workers'][1]
    text += f"| {row['agent']} | {row['day']} | {old['moves']} | {new['moves']} | {sum(new['extra_actions'].values())} | {new['hire_saving']} | {old['tiles_with_multiple_workers']} → {new['tiles_with_multiple_workers']} |\n"
text += '\nThe p355 joint-route day14 contract recovers20wheat,3milk and2fertilizer plus service while saving89 in hires. Existing transactions stay fixed, so extra products are retained inventory, not extra sale revenue. p362 joint-route day14 recovers8wheat and2fertilizer plus service and saves233.\n\n'
text += 'The three timeouts are all p362 joint-route day16; its augmented/two-fewer-worker case solves. This shows search failure does not establish infeasibility. Retry only those three. New construction is outside the added-work contract.\n\n'
text += result['scope'] + '\n'
(RUN / 'RESULTS.md').write_text(text)
print('All12 augmented/two-fewer cases solved; median', result['median_solved_seconds'])
for row in rows:
    a, b = row['source'], row['two_fewer_workers'][1]
    print(row['agent'], row['day'], 'moves', a['moves'], b['moves'], 'extra', sum(b['extra_actions'].values()),
          'multi-worker tiles', a['tiles_with_multiple_workers'], b['tiles_with_multiple_workers'])
