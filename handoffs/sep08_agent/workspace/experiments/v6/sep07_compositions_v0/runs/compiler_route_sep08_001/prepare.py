"""Test finishing useful work at a visited tile, with workforce held fixed."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
PARENT=EXP/'runs/compiler_labor_sep08_001'


def replace(text,old,new):
    assert text.count(old)==1,old
    return text.replace(old,new)


core=RUN/'compiler'
(core/'source').mkdir(parents=True,exist_ok=False)
header=(PARENT/'compiler/source/agent.hpp').read_text().replace('compiler_labor_sep08','compiler_route_sep08')
header=header.replace('int labor_mode=0)', 'int labor_mode=0,int route_mode=0)')
header=header.replace('labor_mode_(labor_mode)', 'labor_mode_(labor_mode),route_mode_(route_mode)')
header=replace(header,'    int labor_mode_=0;', '    int labor_mode_=0,route_mode_=0;')
(core/'source/agent.hpp').write_text(header)
source=(PARENT/'compiler/source/agent.cpp').read_text().replace('compiler_labor_sep08','compiler_route_sep08')
source=replace(source,'    if(labor_mode_<0 || labor_mode_>3)std::abort();',
               '    if(labor_mode_<0 || labor_mode_>3 || route_mode_<0 || route_mode_>3)std::abort();')
source=replace(source,'        pairs[count++]={score,u,j,x,y,travel};', '''        // Count only currently possible field jobs on the same tile. Inputs
        // carried by other workers never contribute to this worker's bundle.
        // This is a score, not a claim that all jobs execute this turn.
        if(job.x>=0) {
            if((route_mode_&1) && travel==0)score+=2000;
            if(route_mode_&2) {
                double bundle=0;
                for(int k=0;k<jobs.size;++k) {
                    const auto& other=jobs.data[k];
                    if(k==j || other.x!=job.x || other.y!=job.y)continue;
                    if(other.input>=0 && own.inv[u][other.input]<=0)continue;
                    if(o.hour+travel+1>other.deadline)continue;
                    bundle+=std::min(other.value,2000.0);
                }
                score+=0.25*bundle/(1+0.3*travel);
            }
        }
        pairs[count++]={score,u,j,x,y,travel};''')
(core/'source/agent.cpp').write_text(source)
(core/'agent.json').write_text(json.dumps({'format_version':1,'name':'compiler_route_sep08_core','header':'source/agent.hpp',
    'type':'compositions::compiler_route_sep08::Agent<>','sources':['source/agent.cpp']},indent=2)+'\n')
(core/'README.md').write_text('# Compiler route score ablations\n\nHiring and service generation remain inherited. Bit1 adds a field-job bonus for a worker already at the tile. Bit2 values other currently possible jobs at the same tile. No persistent target state, extra observation or hidden state is added. See ../LINEAGE.json.\n')
(RUN/'cold_farm.hpp').write_text('#pragma once\n#include "../compiler_labor_sep08_001/cold_farm.hpp"\n')
names=[]
catalog_path=EXP/'configs/league.json'
catalog=json.loads(catalog_path.read_text())
for case in ['mixed','goose','p4','p55']:
    for mode in range(4):
        name=f'compiler_route_{case}_m{mode}'
        folder=RUN/'proposals'/name
        (folder/'source').mkdir(parents=True,exist_ok=False)
        if case.startswith('p'):
            constructor=f'Agent():AgentCore({int(case[1:])},true,true,true,1,true,0,{mode}) {{}}'
        else:
            counts,labor=('2,2,0,7,12,8',1) if case=='mixed' else ('0,0,6,12,0,0',3)
            constructor=f'Agent():AgentCore(compiler_labor_data::cold_farm({counts}),Support{{}},false,false,false,1,true,{labor},{mode}) {{}}'
        (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../cold_farm.hpp"
namespace compositions::{name} {{
class Agent:public compiler_route_sep08::AgentCore {{
public:
    {constructor}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
            'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../compiler/source/agent.cpp']},indent=2)+'\n')
        (folder/'README.md').write_text(f'# {name}\n\nExperimental routing-score mode{mode} on case{case}. Workforce follows the fixed comparison control described in ../../LINEAGE.json. Not promoted.\n')
        assert name not in catalog
        catalog[name]=str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
spec={'created_utc':datetime.now(timezone.utc).isoformat(),'parent':str(PARENT.relative_to(EXP)),
    'parent_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                    [PARENT/'compiler/source/agent.hpp',PARENT/'compiler/source/agent.cpp',PARENT/'cold_farm.hpp',PARENT/'ANALYSIS.json']},
    'variants':names,'discovery':{'seed_start':1000,'seeds':8,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'mode_bits':{'1':'Add2000 to a field job already under the worker.','2':'Add25% of other currently possible same-tile field job values, each capped2000.'},
    'controls':{'mixed':'Previous labor mixed m1 (eight hands).','goose':'Previous labor goose m3 (estimated hands).',
                'p4':'Original program4 with source hiring/layout/service.','p55':'Original program55 with source hiring/layout/service.'},
    'evidence':'With identical source hiring, compiler takes3981 moves versus3200 for source55, loses about$37318 cash, and misses animal service. Source4 has a similar778 extra moves. Earlier persistent-target ablations failed; this experiment values complete visits without retaining a target.',
    'scope':'Local routing-score ablations; no copied new external policy block, no promotion or broad strength claim.'}
(RUN/'LINEAGE.json').write_text(json.dumps(spec,indent=2)+'\n')
(RUN/'run_discovery.py').write_bytes((PARENT/'run_discovery.py').read_bytes())
print('Prepared',len(names),'route-score policies.')
