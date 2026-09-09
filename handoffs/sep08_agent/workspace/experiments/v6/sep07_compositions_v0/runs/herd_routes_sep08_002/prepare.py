from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PARENT = EXP / 'runs/herd_routes_sep08_001'
source = (PARENT / 'routes.hpp').read_text().replace('herd_routes_sep08', 'herd_routes_partial_sep08')
source = source.replace('Action& action,int crop_end_day)', 'Action& action,int crop_end_day,int mode)')
old = '''            // A route takes its complete remaining wheat requirement in one trip.
            if(carried<needed && !shed){a=toward(x,y,std::clamp(x,4,5),std::clamp(y,4,5));continue;}
            const int cell=tiles_[u][cursor_[u]];const auto& t=f.tiles[cell/10][cell%10];'''
new = '''            const int cell=tiles_[u][cursor_[u]];const auto& t=f.tiles[cell/10][cell%10];
            // Partial loads can serve an animal before restocking. Finish that
            // animal's remaining care/harvest/collection after consuming wheat.
            bool restock=mode==1?carried<needed:carried==0 && !t.fed_today;
            if(restock && !shed){a=toward(x,y,std::clamp(x,4,5),std::clamp(y,4,5));continue;}
            if(mode==2 && restock && shed)continue;'''
assert source.count(old) == 1
source = source.replace(old, new).replace('routes_.apply(o,a,CropEndDay)', 'routes_.apply(o,a,CropEndDay,Mode)')
(RUN / 'routes.hpp').write_text(source)
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for case in ['mixed', 'goose', 'p355', 'p362']:
    for mode in [1, 2]:
        name = f'herd_partial_{case}_m{mode}'
        folder = RUN / 'proposals' / name
        (folder / 'source').mkdir(parents=True, exist_ok=False)
        (folder / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../routes.hpp"
#include "../../../../compiler_split_sep08_001/proposals/compiler_split_{case}/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public herd_routes_partial_sep08::Agent<compiler_split_{case}::Agent,{mode}> {{
public:
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (folder / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../../compiler_split_sep08_001/compiler/source/agent.cpp']}, indent=2) + '\n')
        (folder / 'README.md').write_text(f'# {name}\n\nCold {case} farm with late crop-free herd routes. Mode1 is the original complete-load route policy; mode2 permits partial wheat loads and completes service on the current animal before returning. Same composition/hiring/market projection. Source, exact loop witness and scope in ../../LINEAGE.json. Experimental, not promoted.\n')
        assert name not in catalog
        catalog[name] = str(folder.relative_to(ROOT))
        names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
(RUN / 'run_discovery.py').write_bytes((PARENT / 'run_discovery.py').read_bytes())
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(), 'variants': names,
    'parent': 'runs/herd_routes_sep08_001',
    'discovery': {'opponents': ['public_router', 'observed_sale_lead_start_216'], 'seeds': 8, 'seed_start': 1000, 'seat_mode': 'both'},
    'witness': 'Parent p355 seed1000 seat0 worker8 steps678-680 repeatedly reverses with one wheat; another worker consumes the remaining shed supply before its pickup. Recorded in parent coverage_results/herd_routes_p355_m1.coverage.jsonl.',
    'change': 'Allow partial load work, finish current animal services, wait at shed when empty instead of leaving and returning.',
    'scope': 'Only late crop-free routing. Does not solve mixed day18 service gap or expand the composition.',
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in [PARENT / 'routes.hpp', RUN / 'routes.hpp', RUN / 'prepare.py']}}, indent=2) + '\n')
print('Prepared partial-load route comparison.')
