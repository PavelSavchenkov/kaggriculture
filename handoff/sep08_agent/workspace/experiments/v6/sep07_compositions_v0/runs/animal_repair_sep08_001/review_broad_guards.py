"""Check complete guard-trace coverage against the preregistered service gate."""
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path

RUN = Path(__file__).resolve().parent
BROAD = RUN / 'broad_2360000'
protocol = json.loads((BROAD / 'PROTOCOL.json').read_text())
execution = json.loads((BROAD / 'EXECUTION.json').read_text())
assert execution['games'] == protocol['games'] and execution['sources_unchanged']
audit_path = RUN / 'guard_audit_results/AUDIT.json'
audit = json.loads(audit_path.read_text())
traces = {row['name']: row for row in audit['results']}
allowed_fields = {('tile', '38', str(i)) for i in (0, 1, 3, 4, 5)} | {('seed', '0', '0'), ('shed', '0', '0')}
counts = Counter()
patterns = Counter()
contracts = RUN.parent / 'animal_groups_sep08_001/integrated_audit/cow_triple/days'
coop_contracts = []
for day in (28, 29):
    path = contracts / str(day) / 'problem.json'
    problem = json.loads(path.read_text())
    work = [w for w in problem['tile_work'] if w['tile'] == 73]
    assert work == ([{'tile': 73, 'actions': [{'op': 'build_coop', 'arg': -1,
        'quantity': 1, 'output_item': -1, 'output_quantity': 0}]}] if day == 28 else [])
    end = next(t['state'] for t in problem['end_tiles'] if t['tile'] == 73)
    assert end['kind'] == 'coop' and end['animal'] == -1 and end['stored_units'] == 0
    assert not any(m['op'] == 'buy_animal' for m in problem['buy_schedule'])
    coop_contracts.append({'day': day, 'source': str(path),
        'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'work': work, 'end': end})
reviewed, unexpected = [], []
for job in protocol['jobs']:
    if job['a'] == 'empty_sale_slots_m2':
        continue
    name = ('native_' if job['native'] else '') + f"{job['a']}_vs_{job['b']}"
    file = BROAD / (name + '.json.diagnostics.json')
    missed = [row for row in json.loads(file.read_text())['games'] if row['missed_days']]
    if not missed:
        continue
    trace = traces[name]
    assert trace['exact_full_records_equal']
    differences = defaultdict(list)
    for row in trace['differences']:
        differences[int(row['seed']), int(row['seat'])].append(row)
    assert set(differences) == {(row['seed'], row['seat']) for row in missed}
    for row in missed:
        fields = differences[row['seed'], row['seat']]
        days = {int(field['day']) for field in fields}
        assert sum(1 << day for day in days) == row['missed_days']
        first = [field for field in fields if int(field['day']) == min(days)]
        starts_with_weed = len(first) == 1 and all(first[0][key] == value for key, value in
            {'day': '23', 'kind': 'tile', 'item': '38', 'field': '0', 'expected': '0', 'actual': '2'}.items())
        confined = all((field['kind'], field['item'], field['field']) in allowed_fields for field in fields)
        counts[job['a']] += 1
        unused_coop = (job['native'] and job['b'] == 'pass' and row['seed'] == 2364000 and row['seat'] == 0
            and len(fields) == 2 and all(field == dict(day=str(day), family='0', choice='2', leaf='0',
                kind='tile', item='73', field='0', expected=str(expected), actual='2',
                seed='2364000', seat='0') for field, day, expected in zip(fields, (28, 29), (0, 3))))
        if starts_with_weed and confined:
            patterns['day23_tile38_wheat_loss'] += 1
        elif unused_coop:
            patterns['day28_tile73_unused_coop'] += 1
        else:
            unexpected.append({'matchup': name, 'seed': row['seed'], 'seat': row['seat'], 'differences': fields})
    reviewed.append({'matchup': name, 'missed_games': len(missed),
        'diagnostics_sha256': hashlib.sha256(file.read_bytes()).hexdigest()})
report = {'games_in_broad_audit': execution['games'], 'trace_matchups': len(reviewed),
    'missed_games_by_candidate': dict(counts), 'trace_games': audit['games'],
    'all_misses_traced': True, 'unexpected': unexpected,
    'passes_declared_guard_investigation_gate': not unexpected,
    'audit_sha256': hashlib.sha256(audit_path.read_bytes()).hexdigest(), 'reviewed': reviewed,
    'patterns': dict(patterns), 'unused_coop_contracts': coop_contracts,
    'diagnosis': 'Day23 tile38 misses affect wheat state and later wheat stock. Two native PASS games instead have only a day28 tile73 weed blocking construction of an empty coop; day29 has no work on that tile and no animal is ever assigned there in the remaining course. All other checked tiles, animal service states and inventories match. This classifies the failed build; it does not invent a counterfactual cash gain.',
    'scope': 'Checks coverage of the preregistered guard investigation. Complete acceptance requires no unexpected cases. Does not change numerical promotion gates, erase unit faults or claim that all courses execute exactly.'}
(RUN / 'GUARD_REVIEW.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({key: value for key, value in report.items() if key not in ('reviewed', 'unexpected')}, indent=2))
assert not unexpected, 'New guard differences require investigation before candidate selection.'
