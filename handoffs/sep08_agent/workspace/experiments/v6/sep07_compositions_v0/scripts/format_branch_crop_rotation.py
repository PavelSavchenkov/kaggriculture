"""Format two complete crop continuations with the existing observed berry rule."""
import argparse
import hashlib
import json
import os
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('run')
    parser.add_argument('--off', required=True)
    parser.add_argument('--on', required=True)
    parser.add_argument('--name', default='crop_rotation_t2_berry')
    args = parser.parse_args()
    assert all(name.replace('_', '').isalnum() for name in [args.run, args.off, args.on, args.name])
    output = EXP / 'runs' / args.run
    package = output / 'proposals' / args.name
    assert not output.exists()
    folders = {'off': EXP / 'runs' / args.off, 'on': EXP / 'runs' / args.on}
    for folder in folders.values():
        status = json.loads((folder / 'STATUS.json').read_text())
        assert status['status'] == 'compiled' and status['days'] == 17
    entry = [folder / 'days/20/guard.txt' for folder in folders.values()]
    assert entry[0].read_bytes() == entry[1].read_bytes()
    (package / 'source').mkdir(parents=True)
    relative = lambda path: os.path.relpath(path, package / 'source')
    inputs = [Path(__file__), EXP / 'include/crop_branch_sequence.hpp', EXP / 'include/crop_rotation_shop_gate.hpp',
              EXP / 'runs/productive_wheat_rotation_001/source/policy.hpp']
    code = '#pragma once\n'
    for path in [EXP / 'include/crop_branch_sequence.hpp', EXP / 'include/crop_rotation_shop_gate.hpp',
                 EXP / 'runs/crop_value_001/proposals/crop_value_m2_t4/source/agent.hpp']:
        code += '#include "' + relative(path) + '"\n'
    for folder in folders.values():
        for day in range(12, 29):
            code += '#include "' + relative(folder / f'days/{day}/schedule.hpp') + '"\n'
    code += f'namespace compositions::{args.name} {{\n'
    for key, folder in folders.items():
        inputs += [folder / 'RUN.json', folder / 'STATUS.json']
        code += f'inline std::vector<GuardedDay> {key}_days(){{std::vector<GuardedDay> result;\n'
        for day in range(12, 29):
            path = folder / f'days/{day}'
            values = [list(map(int, line.split())) for line in (path / 'guard.txt').read_text().splitlines()]
            assert len(values) == 103 and all(len(row) == 13 for row in values[1:101])
            assert values[0][0] == day
            code += f'{{GuardedDay g;g.plan={{{day},{folder.name}_d{day}::schedule()}};g.quadrants={values[0][1]};\ng.tiles={{{{'
            code += ','.join('{{' + ','.join(map(str, row[1:])) + '}}' for row in values[1:101]) + '}};\n'
            for field, data in [('check', [row[0] for row in values[1:101]]), ('shed', values[101]), ('seeds', values[102])]:
                code += f'g.{field}={{{",".join(map(str, data))}}};\n'
            code += 'result.push_back(std::move(g));}\n'
            inputs += [path / 'guard.txt', path / 'schedule.hpp', path / 'problem.json']
        code += 'return result;}\n'
    code += 'class Course:public CropBranchSequence<crop_value_m2_t4::Agent>{public:Course():CropBranchSequence(off_days(),on_days()){} };\n'
    code += f'class Agent:public CropRotationShopGate<crop_value_m2_t4::Agent,Course>{{public:Agent():CropRotationShopGate(2){{}}static kag::agent::AgentInfo info(){{return {{"{args.name}"}};}}}};}}\n'
    (package / 'source/agent.hpp').write_text(code)
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    manifest = {'format_version': 1, 'name': args.name, 'header': 'source/agent.hpp',
                'type': f'compositions::{args.name}::Agent',
                'sources': ['source/agent.cpp', os.path.relpath(EXP / 'league/top_replay_library/source/agent.cpp', package)]}
    (package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
    record = {'parent': 'crop_value_m2_t4', 'predecessor': 'crop_rotation_t2_current',
              'course_sources': {key: str(folder.relative_to(EXP)) for key, folder in folders.items()},
              'rule': 'At day12 choose tomato course with at least2observed tomato-consuming shops and physical entry match. At day20 retain original weighted berry-demand>=4 rule with two separately compiled complete suffixes.',
              'implementation_lineage': 'CropBranchSequence adapted from productive_wheat_rotation_001/source/policy.hpp; serializers from format_crop_rotations.py and productive wheat format_packages.py; donor crop calendar lineage in source RUN.json.',
              'day20_guard_byte_identical': True, 'status': 'Unpromoted; pending causal, fresh league and operational validation.',
              'source_sha256': {str(path.relative_to(EXP)): hashlib.sha256(path.read_bytes()).hexdigest() for path in inputs}}
    (package / 'IMPORT.json').write_text(json.dumps(record, indent=2) + '\n')
    (output / 'BERRY_ENTRY_PARITY.json').write_text(json.dumps({'day': 20, 'byte_identical': True,
        'sources': {str(path.relative_to(EXP)): hashlib.sha256(path.read_bytes()).hexdigest() for path in entry}}, indent=2) + '\n')
    (package / 'README.md').write_text(f'# {args.name}\n\nThree-tile tomato replacement selected from observed day12 shops; separately compiled day20 berry continuations preserve the existing adaptive fertilizer decision. Source routes, guards and lineage are in IMPORT.json. Unpromoted pending fresh league and operational validation.\n')
    catalog_path = EXP / 'configs/league.json'
    catalog = json.loads(catalog_path.read_text())
    assert args.name not in catalog
    catalog[args.name] = str(package.relative_to(ROOT))
    catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
    print(args.name, 'day20 guards identical')


if __name__ == '__main__':
    main()
