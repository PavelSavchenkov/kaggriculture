from pathlib import Path
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
traces = [json.loads(line) for line in (RUN / 'prefix.json.traces.jsonl').read_text().splitlines()]
trace = next(t for t in traces if t['seed'] == 1005 and t['seat'] == 0)
state = next(s for s in trace['v52']['states'] if s['step'] == 144)
physical = state['physical']
assert physical[:3] == [1, 1, 0] and physical[20:35] == [4, 4] + [0] * 13
assert len(physical) == 1235
source = RUN / 'source'
source.mkdir(exist_ok=True)
header = '#pragma once\n#include <array>\nnamespace compositions::v52_family {\n'
header += 'inline constexpr std::array<int,12> target_shed={' + ','.join(map(str, physical[3:15])) + '};\n'
header += 'inline constexpr std::array<int,5> target_seeds={' + ','.join(map(str, physical[15:20])) + '};\n'
header += 'inline constexpr std::array<std::array<int,12>,100> target_tiles={{\n'
header += ',\n'.join('{{' + ','.join(map(str, physical[i:i+12])) + '}}' for i in range(35, 1235, 12)) + '\n}};\n}\n'
(source / 'entry.hpp').write_text(header)
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
for name, threshold, all_routes in [('v52_transfer_wool2', 2, False), ('v52_transfer_wool4', 4, False),
                                   ('v52_transfer_wool0', 0, False), ('v52_transfer_routes', 0, True)]:
    package = RUN / 'proposals' / name
    assert name not in catalog and not package.exists()
    (package / 'source').mkdir(parents=True)
    (package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/transfer.hpp"\n'
        f'namespace kag::agents::{name}{{class Agent:public compositions::v52_family::Transfer{{public:Agent():Transfer({threshold},{str(all_routes).lower()}){{}}'
        f'static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp', 'type': f'kag::agents::{name}::Agent',
                'sources': ['source/agent.cpp', *[os.path.relpath(EXP / f'league/{dep}/source/agent.cpp', package) for dep in ['top_replay_library', 'public_router']]]}
    (package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
    (package / 'IMPORT.json').write_text(json.dumps({'parent': 'late_value_s32_t0_r05', 'donor': 'public_router_v52',
        'source_lineage': 'league/public_router_v52/IMPORT.json', 'source_data_sha256': hashlib.sha256((EXP / 'league/public_router_v52/source/data.inc').read_bytes()).hexdigest(),
        'entry_observation_seed': 1005, 'entry_observation_seat': 0, 'entry_day': 6, 'wool_demand_minimum': threshold,
        'all_source_routes': all_routes, 'changes': ['Keep incumbent through day5', 'Match donor physical tiles from public own state',
        'Restore missing day6 wheat and seed stock; sell excess fertilizer', 'Donor full suffix after entry; no later incumbent branches'],
        'status': 'Experimental; discovery and physical/output audits pending. Not promoted.',
        'known_rating': 'Unknown; no separate donor license supplied; user authorizes borrowing.'}, indent=2) + '\n')
    (package / 'README.md').write_text(f'# {name}\n\nIncumbent opening plus Thomas V5/2 full suffix from day6 behind observed tile and stock checks. '
        'Restores missing wheat and seed reserves. Exact source attribution and parameters in IMPORT.json. Experimental, unpromoted.\n')
    catalog[name] = str(package.relative_to(ROOT))
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
print('Prepared four transfer packages; physical entry source recorded.')
