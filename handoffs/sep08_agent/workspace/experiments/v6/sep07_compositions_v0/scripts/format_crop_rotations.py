"""Expose complete checked crop rotations through the repository C++ agent API."""
import argparse
import hashlib
import json
import os
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ROOT = EXP.parents[2]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('runs', nargs='+')
    args = parser.parse_args()
    catalog_path = EXP / 'configs/league.json'
    catalog = json.loads(catalog_path.read_text())
    for name in args.runs:
        donor = EXP / 'runs' / name
        status = json.loads((donor / 'STATUS.json').read_text())
        assert status['status'] == 'compiled'
        package = donor / 'agent'
        assert not package.exists() and name not in catalog
        (package / 'source').mkdir(parents=True)
        relative = lambda path: os.path.relpath(path, package / 'source')
        header = '#pragma once\n'
        for path in [EXP / 'include/guarded_sequence.hpp', EXP / 'candidates/investment_context_guarded_001_best/source/agent.hpp']:
            header += '#include "' + relative(path) + '"\n'
        days = sorted((donor / 'days').iterdir(), key=lambda path: int(path.name))
        assert len(days) == status['days']
        for day in days:
            header += '#include "' + relative(day / 'schedule.hpp') + '"\n'
        header += f'namespace compositions::{name} {{\ninline std::vector<GuardedDay> days(){{std::vector<GuardedDay> result;\n'
        for folder in days:
            values = [list(map(int, line.split())) for line in (folder / 'guard.txt').read_text().splitlines()]
            assert len(values) == 103 and all(len(row) == 13 for row in values[1:101])
            day, quadrants = values[0]
            header += f'{{GuardedDay g;g.plan={{{day},{name}_d{day}::schedule()}};g.quadrants={quadrants};\ng.tiles={{{{'
            header += ','.join('{{' + ','.join(map(str, row[1:])) + '}}' for row in values[1:101]) + '}};\n'
            for key, data in [('check', [row[0] for row in values[1:101]]), ('shed', values[101]), ('seeds', values[102])]:
                header += f'g.{key}={{{",".join(map(str, data))}}};\n'
            header += 'result.push_back(std::move(g));}\n'
        header += f'return result;}}\nclass Agent:public GuardedSequenceAgent<investment_context_guarded_001_best::Agent>{{public:Agent():GuardedSequenceAgent(days()){{}}static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n'
        (package / 'source/agent.hpp').write_text(header)
        (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
                    'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', os.path.relpath(EXP / 'league/top_replay_library/source/agent.cpp', package)]}
        (package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
        run = json.loads((donor / 'RUN.json').read_text())
        files = [donor / 'RUN.json', donor / 'STATUS.json', Path(__file__),
                 *[p for d in days for p in [d / 'guard.txt', d / 'schedule.hpp', d / 'problem.json']]]
        lineage = {'parent': run['source_parent'], 'lineage': run['lineage'], 'command': run['command'],
                   'status': 'Unpromoted complete crop replacement; see paired SCREEN.json and exact games.',
                   'formatter_origin': 'Adapted from scripts/format_crop_sequences.py; no policy change.',
                   'files_sha256': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}}
        (package / 'IMPORT.json').write_text(json.dumps(lineage, indent=2) + '\n')
        (package / 'README.md').write_text(f'# {name}\n\nComplete dated tomato replacement over the submitted investment-context parent. All changed day contracts and routes are retained in ../days; RUN.json records cell selection, seed/trade options and frozen compiler inputs. The first day must match before entry, and every subsequent day checks its physical state. Guards do not prove future financing. See ../SCREEN.json for discovery results and IMPORT.json for donor/implementation lineage. No promotion or submission.\n')
        catalog[name] = str(package.relative_to(ROOT))
        print(name)
    catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')


if __name__ == '__main__':
    main()
