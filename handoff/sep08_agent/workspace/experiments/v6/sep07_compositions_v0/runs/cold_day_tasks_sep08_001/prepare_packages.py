"""Package successful task-generated days behind the unchanged reactive suffix."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
results=json.loads((RUN/'compiled/RESULTS.json').read_text())
assert len(results)==3 and all(r['solved'] and r['full_engine_endpoint_equal'] for r in results)
parts=['#pragma once\n#include <array>\n#include <vector>\n#include "agents/common/api/agent_api.hpp"\n#include "../../include/composition.hpp"\nnamespace compositions::cold_day_tasks {\n',
       '#include "compiled/plan.inc"\n']
for mode in range(3):
    words=[int(v) for v in (RUN/f'compiled/mode{mode}/schedule.txt').read_text().split()]
    parts.append(f'inline std::array<kag::Action,24> make_day{mode}() {{constexpr int data[]={{'+','.join(map(str,words))+'''};
std::array<kag::Action,24> result;const int* p=data;
for(auto& a:result){a.n_units=*p++;a.n_orders=*p++;
for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
a.finalize();}return result;}
''')
parts.append('inline const std::array<std::array<kag::Action,24>,3> days={make_day0(),make_day1(),make_day2()};\n}\n')
(RUN/'data.hpp').write_text(''.join(parts))
(RUN/'policy.hpp').write_text('''#pragma once
#include "../compiler_placement_sep08_001/compiler/source/agent.hpp"
#include "data.hpp"
namespace compositions::cold_day_tasks {
template<int Mode> class Agent {
    compiler_placement_sep08::AgentCore base_;
    bool active_=false;
public:
    Agent():base_(lives,Support{},true,false,false,1,true,1,0) {}
    static kag::agent::AgentInfo info(){return {"cold_day_tasks"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);active_=Mode>=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a){
        base_.act(o,budget,a);
        if constexpr(Mode>=0)if(active_ && o.day==0){
            const auto& planned=days[Mode][o.hour];
            if(planned.n_units==o.self().n_units)a=planned;
            else active_=false;
        }
    }
};
}
''')
catalog_path=EXP/'configs/league.json'
catalog=json.loads(catalog_path.read_text());names=[]
for mode,suffix in [(-1,'control'),(0,'full'),(1,'sheep_service'),(2,'establish')]:
    name='cold_day_tasks_'+suffix;folder=RUN/'proposals'/name
    (folder/'source').mkdir(parents=True,exist_ok=False)
    (folder/'source/agent.hpp').write_text('#pragma once\n#include "../../../policy.hpp"\n')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::cold_day_tasks::Agent<{mode}>',
        'sources':['source/agent.cpp','../../../compiler_placement_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
    (folder/'README.md').write_text(f'# {name}\n\nMode{mode}: task-generated day0, followed by the inherited reactive cold-farm compiler with explicit owned-first layout and eight-hand hiring rule. Mode-1 is the exact unchanged control. The other modes respectively serve all animals, only sheep, or no animals onday0. Full-season effects and opponent funding still require validation. See ../../README.md and ../../LINEAGE.json.\n')
    assert name not in catalog;catalog[name]=str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,
    'parent':'runs/compiler_placement_sep08_001/proposals/compiler_placement_mixed_m1',
    'sources':json.loads((RUN/'SOURCE_HASHES.json').read_text()),
    'generated_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [RUN/'data.hpp',RUN/'policy.hpp',RUN/'compiled/plan.inc']},
    'solver_results':results,'discovery':{'seed_start':1000,'seeds':32,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216','pass']},
    'scope':'First-day task compiler from raw dated cold lives; source-free worker routes. No strong-agent promotion. Skipped care may change production and is evaluated, not assumed free.'},indent=2)+'\n')
print('Prepared four complete C++ policies including exact control.')
