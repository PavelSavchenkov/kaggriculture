"""Serialize checked calendars, not an executable Python game policy."""
from pathlib import Path
import hashlib
import json
import sys

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
SCHEDULES = EXP / 'runs/late_animal_schedule_001'
COURSES = EXP / 'runs/late_animal_rotation_001'
sys.path.insert(0, str(SCHEDULES))
from format_packages import schedule

out = RUN / 'source/data.hpp'
assert not out.exists()
code = '#pragma once\n#include "model.hpp"\nnamespace compositions::late_portfolio {\n'
hashes = {}
reuse = []
def changes_for(species, leaf):
    mapping = None
    if species == 'goose':
        mapping = SCHEDULES / f'{leaf}_terminal_no_care_audit.txt'
    elif species != 'control':
        mapping = SCHEDULES / f'species30/{species}_{leaf}.txt'
    return {int(day): Path(path) for day, path in (line.split() for line in mapping.read_text().splitlines())} if mapping else {}

def hire_count(path):
    count = 0
    for line in path.read_text().splitlines():
        row = list(map(int, line.split()))
        offset = 2 + 3 * row[0]
        for p in range(offset, len(row), 3):
            if row[p] == 1:
                count += row[p + 2]
    return count

for species in ['control', 'goose', 'cow', 'sheep']:
    code += f'inline Calendar make_{species}(){{Calendar result;\n'
    for leaf_index, leaf in enumerate(['off', 'on']):
        changes = changes_for(species, leaf)
        for day in range(13, 30):
            folder = COURSES / f'{species}_c31_d13_{leaf}_002/days/{day}'
            guard = folder / 'guard.txt'
            actions = changes.get(day, folder / 'actions.txt')
            if day < 20:
                alternatives = []
                for prefix_leaf in ['off', 'on']:
                    prefix_folder = COURSES / f'{species}_c31_d13_{prefix_leaf}_002/days/{day}'
                    assert (prefix_folder / 'guard.txt').read_bytes() == guard.read_bytes()
                    assert (prefix_folder / 'problem_h0.json').read_bytes() == (folder / 'problem_h0.json').read_bytes()
                    alternatives.append(changes_for(species, prefix_leaf).get(day, prefix_folder / 'actions.txt'))
                actions = min(alternatives, key=hire_count)
                if leaf_index == 0:
                    reuse.append({'species': species, 'day': day, 'actions': str(actions.relative_to(EXP)), 'hires': hire_count(actions), 'physical_contracts_byte_identical': True})
            values = [list(map(int, line.split())) for line in guard.read_text().splitlines()]
            assert len(values) == 103 and values[0][0] == day
            code += f'{{GuardedDay g;g.plan={{{day},{schedule(actions)}}};g.quadrants={values[0][1]};\ng.tiles={{{{'
            code += ','.join('{{' + ','.join(map(str, row[1:])) + '}}' for row in values[1:101]) + '}};\n'
            for field, row in [('check', [row[0] for row in values[1:101]]), ('shed', values[101]), ('seeds', values[102])]:
                code += f'g.{field}={{{",".join(map(str, row))}}};\n'
            code += f'result.days[{leaf_index}].push_back(std::move(g));}}\n'
            for path in [guard, actions]:
                hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
    code += 'prepare(result);return result;}\n'
code += 'inline const std::array<Calendar,4>& calendars(){static const std::array<Calendar,4> value{make_control(),make_goose(),make_cow(),make_sheep()};return value;}\n}\n'
out.write_text(code)
(RUN / 'DATA_LINEAGE.json').write_text(json.dumps({'source_sha256': hashes, 'serializer': str((SCHEDULES / 'format_packages.py').relative_to(EXP)),
    'parents': ['runs/late_animal_rotation_001/SOURCE_LINEAGE.json', 'runs/late_animal_schedule_001/TERMINAL_AUDIT.json', 'runs/late_animal_schedule_001/species30/AUDITS.json'],
    'prefix_reuse': reuse,
    'scope': 'Immutable complete calendars and future own intended trades. No future prices, opponent inventory or seed. Cheapest known off/on route used before observed day20 choice, only with identical physical day contracts.'}, indent=2) + '\n')
print('Serialized4 calendars,', len(hashes), 'source files,', out.stat().st_size, 'bytes.')
