from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
OLD = EXP / 'runs/joint_day_routes_sep08_001'
source = (OLD / 'routes.hpp').read_text()
source = source.replace('#include "compiler/source/agent.hpp"', '#include "../joint_day_routes_sep08_001/compiler/source/agent.hpp"')
source = source.replace('namespace compositions::joint_day_routes', 'namespace compositions::joint_resource_routes')
helper = '''// Required initial stock follows route order: local harvest/collection may
// supply later input tasks, but later output cannot fund an earlier task.
inline std::array<int,N_ITEMS> required_inputs(const agent::AgentObservation& o,const Context& c,
        const std::array<int,100>& route,int size,int start=0) {
    std::array<int,N_ITEMS> balance{},required{};
    for(int offset=0;offset<size;++offset) {
        int cell=route[(start+offset)%size];
        for(int j=0;j<c.task_count[cell];++j) {
            const auto& task=c.tasks[cell][j];
            if(task.action.op==OP_COLLECT_FERTILIZER)++balance[FERTILIZER];
            if(task.action.op==OP_HARVEST) {
                const auto& tile=o.self().tiles[cell/10][cell%10];
                if(tile.kind==T_PLANT && tile.what==WHEAT)balance[WHEAT]+=tile.yield_units;
            }
            if(task.input>=0){--balance[task.input];required[task.input]=std::max(required[task.input],-balance[task.input]);}
        }
    }
    return required;
}

template<int Mode> class Routes {'''
assert source.count('class Routes {') == 1
source = source.replace('class Routes {', helper)
anchor = '        int x=o.self().pos_x[u],y=o.self().pos_y[u],time=0,pickups=0;'
source = source.replace(anchor, '''        if constexpr(Mode&1){auto required=required_inputs(o,c,route,size);std::copy_n(required.begin(),N_ITEMS,needs);}
''' + anchor + '''
        if constexpr(Mode&4)for(int item=0;item<N_ITEMS;++item)
            time+=24*std::max(0,needs[item]-int(o.own.inv[u][item])-int(o.own.shed[item]));''')
anchor = '            int target=-1;'
source = source.replace(anchor, '''            std::array<int,N_ITEMS> pickup_need{};std::copy_n(needs,N_ITEMS,pickup_need.begin());
            if constexpr(Mode&2)pickup_need=required_inputs(o,c,routes_[u],sizes_[u],cursor_[u]);
''' + anchor)
source = source.replace('if(needs[item]>o.own.inv[u][item] && available[item]>0)', 'if(pickup_need[item]>o.own.inv[u][item] && available[item]>0)')
source = source.replace('needs[item]-o.own.inv[u][item]>needs[input]-o.own.inv[u][input]', 'pickup_need[item]-o.own.inv[u][item]>pickup_need[input]-o.own.inv[u][input]')
source = source.replace('std::min(available[input],needs[input]-int(o.own.inv[u][input]))', 'std::min(available[input],pickup_need[input]-int(o.own.inv[u][input]))')
source = source.replace('template<class Base,int StartDay> class Agent', 'template<class Base,int StartDay,int Mode> class Agent').replace('    Routes routes_;', '    Routes<Mode> routes_;')
(RUN / 'routes.hpp').write_text(source)
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for case in ['mixed', 'goose', 'p355', 'p362', 'p4', 'p55']:
    for mode in [0, 1, 2, 3, 7]:
        name = f'joint_resources_{case}_m{mode}'
        path = RUN / 'proposals' / name
        (path / 'source').mkdir(parents=True, exist_ok=False)
        (path / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../routes.hpp"
#include "../../../../joint_day_routes_sep08_001/bases/{case}/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public joint_resource_routes::Agent<joint_base_{case}::Agent,14,{mode}> {{
public:
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (path / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (path / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../../joint_day_routes_sep08_001/compiler/source/agent.cpp']}, indent=2) + '\n')
        (path / 'README.md').write_text(f'# {name}\n\nJoint mixed-day routes from day14, mode{mode}. Bit1 credits earlier route wheat/fertilizer output when costing required initial stock; bit2 applies those stock requirements to withdrawals; bit4 penalizes route inputs absent from worker/shed. Mode0 preserves old day14 routing. Existing biological tasks and actual-action market projection retained. Resource estimates are heuristic, not a feasibility proof. Exact provenance in ../../LINEAGE.json.\n')
        assert name not in catalog
        catalog[name] = str(path.relative_to(ROOT))
        names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
(RUN / 'run_discovery.py').write_bytes((OLD / 'run_discovery.py').read_bytes())
paths = [OLD / 'routes.hpp', OLD / 'compiler/source/agent.hpp', OLD / 'compiler/source/agent.cpp', RUN / 'routes.hpp', RUN / 'prepare.py']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(), 'variants': names,
    'parent': 'runs/joint_day_routes_sep08_001, mode1 starting day14',
    'evidence': ['runs/joint_day_routes_sep08_001/ANALYSIS.json', 'runs/joint_day_routes_sep08_001/diagnostics/joint_routes_p355_m1_1000_s0.jsonl', 'day_solver/docs/algorithm.md (persistent root resource)'],
    'hypothesis': 'Existing routes charge every input to a shed trip and ignore fertilizer collection/wheat harvest that can supply later route tasks. Day-solver construction explicitly uses cumulative cargo. Apply that requirement calculation to cost, runtime withdrawals and current-stock scarcity separately.',
    'limits': ['Currently enabled tasks omit some later dependent work; no complete-day guarantee.', 'Per-route stock availability does not solve shared-resource conflicts across routes.', 'This compiler experiment must not replace broader composition search or incumbent improvements.'],
    'discovery': {'opponents': ['public_router', 'observed_sale_lead_start_216'], 'seeds': 8, 'seed_start': 1000, 'seat_mode': 'both'},
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}, indent=2) + '\n')
print('Prepared30 cumulative-input route/control policies.')
