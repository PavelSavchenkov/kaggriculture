"""Freeze checked animal courses into repository-format, observation-only agents."""
from pathlib import Path
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
COURSES = EXP / 'runs/late_animal_rotation_001'
BASE = EXP / 'runs/opening_market_search_001/proposals/opening_q32_b13_v1/source/agent.hpp'


def schedule(path):
    rows = [list(map(int, line.split())) for line in path.read_text().splitlines()]
    assert len(rows) == 24
    for row in rows:
        assert len(row) == 2 + 3 * (row[0] + row[1])
    data = ',\n'.join(','.join(map(str, row)) for row in rows)
    return '''[]{constexpr int data[]={''' + data + '''};
        std::array<kag::Action,24> result;int p=0;
        for(auto& a:result){a.n_units=data[p++];a.n_orders=data[p++];
            for(int u=0;u<a.n_units;++u){a.units[u].op=data[p++];a.units[u].arg=data[p++];a.units[u].n=data[p++];}
            for(int s=0;s<a.n_orders;++s){a.orders[s].op=data[p++];a.orders[s].item=data[p++];a.orders[s].n=data[p++];}
            a.finalize();}return result;}()'''


def main():
    specifications = [('late_goose_initial', 'goose', False, 4),
                      ('late_goose_optimized', 'goose', True, 4),
                      ('late_goose_optimized_off', 'goose', True, 1000),
                      ('late_goose_optimized_on', 'goose', True, 0),
                      ('late_crop_control', 'control', False, 4),
                      ('late_cow_initial', 'cow', False, 4),
                      ('late_sheep_initial', 'sheep', False, 4)]
    catalog_path = EXP / 'configs/league.json'
    catalog = json.loads(catalog_path.read_text())
    for name, species, optimized, threshold in specifications:
        package = RUN / 'proposals' / name
        assert not package.exists() and name not in catalog
        source = package / 'source'
        source.mkdir(parents=True)
        relative = lambda path: os.path.relpath(path, source)
        code = '#pragma once\n'
        for path in [EXP / 'include/crop_branch_sequence.hpp', BASE]:
            code += '#include "' + relative(path) + '"\n'
        code += f'namespace kag::agents::{name} {{\nusing compositions::GuardedDay;\n'
        hashes = {}
        entries = []
        maps = {}
        for leaf in ['off', 'on']:
            folder = COURSES / f'{species}_c31_d13_{leaf}_002'
            status = json.loads((folder / 'STATUS.json').read_text())
            assert status['status'] == 'compiled' and status['days'] == 17
            entries.append((folder / 'days/20/guard.txt').read_bytes())
            changes = {}
            if optimized:
                mapping = RUN / f'{leaf}_terminal_no_care_audit.txt'
                for line in mapping.read_text().splitlines():
                    day, path = line.split()
                    assert int(day) not in changes
                    changes[int(day)] = Path(path)
                maps[leaf] = str(mapping.relative_to(EXP))
            code += f'inline std::vector<GuardedDay> {leaf}_days() {{std::vector<GuardedDay> result;\n'
            for day in range(13, 30):
                folder_day = folder / f'days/{day}'
                guard_path = folder_day / 'guard.txt'
                action_path = changes.get(day, folder_day / 'actions.txt')
                values = [list(map(int, line.split())) for line in guard_path.read_text().splitlines()]
                assert len(values) == 103 and all(len(row) == 13 for row in values[1:101])
                assert values[0][0] == day
                code += f'{{GuardedDay g;g.plan={{{day},{schedule(action_path)}}};g.quadrants={values[0][1]};\ng.tiles={{{{'
                code += ','.join('{{' + ','.join(map(str, row[1:])) + '}}' for row in values[1:101]) + '}};\n'
                for field, row in [('check', [row[0] for row in values[1:101]]), ('shed', values[101]), ('seeds', values[102])]:
                    code += f'g.{field}={{{",".join(map(str, row))}}};\n'
                code += 'result.push_back(std::move(g));}\n'
                for path in [guard_path, action_path, folder_day / 'problem.json']:
                    hashes[str(path.relative_to(EXP))] = hashlib.sha256(path.read_bytes()).hexdigest()
            code += 'return result;}\n'
        assert entries[0] == entries[1]
        code += 'class Agent:public compositions::CropBranchSequence<compositions::opening_q32_b13_v1::Agent>{public:\n'
        code += f'Agent():CropBranchSequence(off_days(),on_days(),20,{threshold}){{}}\n'
        code += f'static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n'
        (source / 'agent.hpp').write_text(code)
        (source / 'agent.cpp').write_text('#include "agent.hpp"\n')
        manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
                    'type': f'kag::agents::{name}::Agent', 'sources': ['source/agent.cpp']}
        for dependency in ['top_replay_library', 'public_router']:
            manifest['sources'].append(os.path.relpath(EXP / f'league/{dependency}/source/agent.cpp', package))
        (package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
        record = {'parent': 'opening_q32_b13_v1', 'species': species, 'day': 13, 'cell': 31,
                  'optimized_routes': optimized, 'day20_berry_demand_threshold': threshold,
                  'day20_entry_guards_identical': True, 'replacement_maps': maps,
                  'status': 'Unpromoted; package parity and broad league validation pending.',
                  'lineage': ['runs/late_animal_rotation_001/SOURCE_LINEAGE.json',
                              'runs/late_animal_rotation_001/COMPILE_V2.json',
                              'runs/late_animal_schedule_001/COMBINED30_AUDIT.json',
                              'runs/late_animal_schedule_001/TERMINAL_AUDIT.json'],
                  'source_sha256': hashes,
                  'scope': 'Complete day13..29 course guarded by observable physical state. Real future berry branch uses shops observed at day20. Force-leaf variants are diagnostic. Initial cow/sheep solver costs are achievable costs, not proven minima.'}
        (package / 'IMPORT.json').write_text(json.dumps(record, indent=2) + '\n')
        (package / 'README.md').write_text(f'# {name}\n\nDay13 replacement of crop cycles on tile(1,3) with {species}; control retains crops. Parent opening_q32_b13_v1 and all earlier adaptive choices remain. Exact physical entry guards and complete future schedules; day20 berry threshold{threshold}. See IMPORT.json for route sources and lineage. Unpromoted pending package and league checks.\n')
        catalog[name] = str(package.relative_to(ROOT))
    catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
    print('Formatted', len(specifications), 'C++ WIP agents.')


if __name__ == '__main__':
    main()
