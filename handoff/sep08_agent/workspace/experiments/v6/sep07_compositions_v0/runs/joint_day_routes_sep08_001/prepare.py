from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
OLD = EXP / 'runs/compiler_split_sep08_001'
folder = RUN / 'compiler/source'
folder.mkdir(parents=True, exist_ok=False)
header = (OLD / 'compiler/source/agent.hpp').read_text().replace('compiler_split_sep08', 'joint_day_core')
header = header.replace('    struct MarketContext {', '''    struct TileTask {kag::UnitAction action;int input;double value;int deadline;};
    struct MarketContext {
        std::array<std::array<TileTask,6>,100> tasks{};
        std::array<int,100> task_count{};
        std::array<bool,100> sites{};''')
(folder / 'agent.hpp').write_text(header)
source = (OLD / 'compiler/source/agent.cpp').read_text().replace('compiler_split_sep08', 'joint_day_core')
anchor = '    struct Pair {double score;int worker,job,x,y,travel;};'
assert source.count(anchor) == 1
source = source.replace(anchor, '''    context.task_count.fill(0);context.sites.fill(false);
    for(int cell=0;cell<100;++cell) {
        const auto& t=farm.tiles[cell/10][cell%10];
        context.sites[cell]=desired[cell]>=0 || t.kind==T_PLANT || t.has_animal;
    }
    for(int j=0;j<jobs.size;++j) {
        const auto& job=jobs.data[j];if(job.x<0)continue;
        int cell=job.y*10+job.x,n=context.task_count[cell]++;
        if(n>=6)std::abort();
        context.tasks[cell][n]={job.action,job.input,job.value,job.deadline};
        context.sites[cell]=true;
    }
''' + anchor)
(folder / 'agent.cpp').write_text(source)
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for case in ['mixed', 'goose', 'p355', 'p362', 'p4', 'p55']:
    base = RUN / 'bases' / case / 'source'
    base.mkdir(parents=True, exist_ok=False)
    text = (OLD / f'proposals/compiler_split_{case}/source/agent.hpp').read_text()
    text = text.replace('compiler_split_sep08', 'joint_day_core').replace(f'compiler_split_{case}', f'joint_base_{case}')
    (base / 'agent.hpp').write_text(text)
    for mode, start in [(0, 30), (1, 14), (2, 0)]:
        name = f'joint_routes_{case}_m{mode}'
        path = RUN / 'proposals' / name
        (path / 'source').mkdir(parents=True, exist_ok=False)
        (path / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../routes.hpp"
#include "../../../bases/{case}/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public joint_day_routes::Agent<joint_base_{case}::Agent,{start}> {{
public:
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (path / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (path / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../compiler/source/agent.cpp']}, indent=2) + '\n')
        (path / 'README.md').write_text(f'# {name}\n\nJoint tile-task routes on {case}. Mode0 unchanged compiler, mode1 starts day14, mode2 starts day0. At hour3 after initial hires, assign crop/animal/construction tiles and required inputs to persistent worker routes. Existing dated biology/service rules and market projection retained. No source worker route, seed or hidden observation. Greedy assignment does not certify daily feasibility. Source and scope in ../../LINEAGE.json.\n')
        assert name not in catalog
        catalog[name] = str(path.relative_to(ROOT))
        names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
(RUN / 'run_discovery.py').write_bytes((OLD / 'run_discovery.py').read_bytes())
paths = [OLD / 'compiler/source/agent.cpp', OLD / 'compiler/source/agent.hpp', folder / 'agent.cpp', folder / 'agent.hpp', RUN / 'routes.hpp', RUN / 'prepare.py']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'variants': names, 'discovery': {'opponents': ['public_router', 'observed_sale_lead_start_216'], 'seed_start': 1000, 'seeds': 8, 'seat_mode': 'both'},
    'parent': 'runs/compiler_split_sep08_001',
    'evidence': ['runs/dated_day_service_sep08_002/RESULTS.json', 'runs/compiler_feed_bundle_sep08_001/ANALYSIS.json', 'runs/herd_routes_sep08_001/COVERAGE.json'],
    'ideas': ['Expose the existing currently legal tile tasks without changing their dated biological/service rules.', 'Assign complete mixed crop/animal/construction routes and input withdrawals jointly, preserving worker ownership.', 'Revisit newly enabled dependent tasks; reserve seeds and shed stock across same-turn worker actions before exact market projection.'],
    'limits': ['Greedy insertion uses currently visible task counts, not all dependent future tasks; measured lower-bound error remains possible.', 'No daily feasibility or cash guarantee; root-engine full games and service profiles are required.', 'The local cold families and attributed recorded compositions remain unchanged; this isolates their compiler.'],
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}, indent=2) + '\n')
print('Prepared18 mixed-day route/control policies.')
