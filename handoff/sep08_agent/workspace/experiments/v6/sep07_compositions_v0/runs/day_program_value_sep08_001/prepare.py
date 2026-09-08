from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
source = EXP / 'runs/day_programs_sep08_001'
checks = json.loads((source / 'OPERATIONAL_CHECKS.json').read_text())
assert checks['games'] == 104
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for case in ['p355', 'p362']:
    for mode, days in [(0, 0), (1, 1), (2, 3), (3, 30)]:
        name = f'day_value_{case}_m{mode}'
        path = RUN / 'proposals' / name
        (path / 'source').mkdir(parents=True, exist_ok=False)
        if mode == 0:
            code = f'''#pragma once
#include "../../../../day_programs_sep08_001/proposals/day_program_{case}_m3/source/agent.hpp"
namespace compositions::{name} {{class Agent:public day_program_{case}_m3::Agent {{public:static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}
'''
        else:
            calls = ','.join(f'day_program_data::{case}_d{d}_source{m}()' for d in [14, 15, 16] for m in [0, 1])
            code = f'''#pragma once
#include "../../../policy.hpp"
#include "../../../../joint_day_routes_sep08_001/proposals/joint_routes_{case}_m0/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public day_program_value::Agent<joint_routes_{case}_m0::Agent,{days}> {{
public:Agent():day_program_value::Agent<joint_routes_{case}_m0::Agent,{days}>(std::vector<day_programs::Program>{{{calls}}}) {{}}
static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
'''
        (path / 'source/agent.hpp').write_text(code)
        (path / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (path / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../../joint_day_routes_sep08_001/compiler/source/agent.cpp']}, indent=2)+'\n')
        (path / 'README.md').write_text(f'# {name}\n\nMode{mode}; horizon{days}days (30 means remaining season). Mode0 is the unchanged relaxed day-program controller. Other modes compare the current compiler continuation and each feasible day program from observed state. Forecasts assume rival PASS, no new shops and no random weeds; future own actions use a copied compiler. Short forecasts mark remaining products/seeds at common observed prices and omit unfinished biological value. Complete forecasts use final cash. Initial-day program legality is checked, and actual failures return to the base compiler. Per-instance state, deterministic node budget and deadline checks. Experimental; see ../../LINEAGE.json.\n')
        assert name not in catalog
        catalog[name] = str(path.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2)+'\n')
paths = [RUN / 'prepare.py', RUN / 'policy.hpp', source / 'model.hpp', source / 'program.hpp', source / 'days.hpp',
         EXP / 'runs/joint_day_routes_sep08_001/compiler/source/agent.cpp', EXP / 'runs/joint_day_routes_sep08_001/compiler/source/agent.hpp']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(), 'variants': names,
    'source': 'Reuse the same twelve locally solved day programs and verified observation model. New local C++ continuation comparison; no external policy code added.',
    'reason': 'Physical reuse saved hires but sometimes lost later animal output. Compare a program with the unchanged compiler using 1-day, 3-day and remaining-season own continuations.',
    'discovery': {'seed_start': 1000, 'seeds': 8, 'seat_mode': 'both', 'opponents': ['public_router', 'observed_sale_lead_start_216', 'matching_original_compiler', 'pass']},
    'limits': 'Only two cold farms/days14..16. Forecast ignores rival actions, future shops and random weeds; shorter horizons omit unfinished biology. Must measure ranking and full-game outcomes, not infer strength from forecast.',
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}, indent=2)+'\n')
# Keep the same full-game panel so old relaxed controls and original compiler
# records can be compared exactly. Evaluation only, no policy decisions in Python.
runner = (source / 'run_discovery.py').read_text().replace('day_program_', 'day_value_')
(RUN / 'run_discovery.py').write_text(runner)
print('Prepared eight continuation-value/control C++ policies.')
