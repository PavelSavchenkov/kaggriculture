from pathlib import Path
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
specs = [('late_value_s32_t0_r0', 32, 0, 0, 0, -1),
         ('late_value_s32_t100_r0', 32, 100, 0, 0, -1),
         ('late_value_s32_t0_r1', 32, 0, 1, 0, -1)]
specs += [(f'late_force_{name}', 32, 0, 0, 0, i) for i, name in enumerate(['wait', 'goose', 'cow', 'sheep'])]
for name, *parameters in specs:
    package = RUN / 'proposals' / name
    assert name not in catalog and not package.exists()
    (package / 'source').mkdir(parents=True)
    code = '#pragma once\n#include "../../../source/policy.hpp"\n'
    code += f'namespace kag::agents::{name}{{class Agent:public compositions::late_portfolio::Policy{{public:Agent():Policy({",".join(map(str, parameters))}){{}}static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n'
    (package / 'source/agent.hpp').write_text(code)
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp', 'type': f'kag::agents::{name}::Agent',
                'sources': ['source/agent.cpp', os.path.relpath(EXP / 'league/top_replay_library/source/agent.cpp', package),
                            os.path.relpath(EXP / 'league/public_router/source/agent.cpp', package)]}
    (package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (package / 'README.md').write_text(f'# {name}\n\nFour-way day13 choice: retain crops, goose, cow or sheep. Preserves parent tomato intention and observed day20 berry branch. Parameters(samples, minimum gain, risk weight, own-profit weight, forced choice)={parameters}. Exact calendars and source hashes: ../../DATA_LINEAGE.json. Local market estimator: ../../source/model.hpp and policy.hpp. All inference inputs are public/own observations; future shops are conditionally sampled. Unpromoted; validation pending.\n')
    (package / 'IMPORT.json').write_text(json.dumps({'parent': 'opening_q32_b13_v1', 'external_calendar_lineage': '../../DATA_LINEAGE.json',
        'local_changes': 'C++ whole-farm trade valuation, actual fixed hire/seed/animal costs, conditional unknown shop samples, four complete courses, inherited parent intent and berry rules.',
        'parameters': parameters, 'status': 'Unpromoted; exact policy parity and fresh validation required.'}, indent=2) + '\n')
    catalog[name] = str(package.relative_to(ROOT))
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
print('Prepared', len(specs), 'C++ WIP agents.')
