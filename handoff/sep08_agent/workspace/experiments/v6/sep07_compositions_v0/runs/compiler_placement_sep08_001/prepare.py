"""Compare greedy distance placement with filling already allocated land first."""
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
header=(PARENT/'compiler/source/agent.hpp').read_text().replace('compiler_labor_sep08','compiler_placement_sep08')
header=header.replace('int labor_mode=0)', 'int labor_mode=0,int placement_mode=0)')
header=header.replace('labor_mode_(labor_mode)', 'labor_mode_(labor_mode),placement_mode_(placement_mode)')
header=replace(header,'    int labor_mode_=0;', '    int labor_mode_=0,placement_mode_=0;')
(core/'source/agent.hpp').write_text(header)
source=(PARENT/'compiler/source/agent.cpp').read_text().replace('compiler_labor_sep08','compiler_placement_sep08')
source=replace(source,'        for(auto& intent:intents_) {', '''        int allocated_quadrants=1;
        for(auto& intent:intents_) {''')
source=replace(source,'            int best=-1;double score=1e100;', '''            int best=-1;double score=1e100;
            bool free_owned=false;
            for(int cell=0;cell<100;++cell)
                free_owned|=release[cell]<=intent.start && quadrant_of(cell%10,cell/10,10)<allocated_quadrants;''')
source=replace(source,'                const double value=5*q+', '''                if(placement_mode_ && free_owned && q>=allocated_quadrants)continue;
                const double value=5*q+''')
source=replace(source,'            intent.x=best%10;intent.y=best/10;', '''            allocated_quadrants=std::max(allocated_quadrants,quadrant_of(best%10,best/10,10)+1);
            intent.x=best%10;intent.y=best/10;''')
source=replace(source,'    if(labor_mode_<0 || labor_mode_>3)std::abort();',
               '    if(labor_mode_<0 || labor_mode_>3 || placement_mode_<0 || placement_mode_>1)std::abort();')
(core/'source/agent.cpp').write_text(source)
(core/'agent.json').write_text(json.dumps({'format_version':1,'name':'compiler_placement_sep08_core','header':'source/agent.hpp',
    'type':'compositions::compiler_placement_sep08::Agent<>','sources':['source/agent.cpp']},indent=2)+'\n')
(core/'README.md').write_text('# Placement comparison\n\nMode0 preserves inherited placement. Mode1 fills previously allocated quadrants before opening another. Dated counts, service, hiring and purchase priority are unchanged. The rule is an experiment, not a proof of optimal land or routing.\n')
catalog_path=EXP/'configs/league.json'
catalog=json.loads(catalog_path.read_text())
names=[]
for case in ['mixed','goose','dairy','wool']:
    for mode in range(2):
        name=f'compiler_placement_{case}_m{mode}'
        folder=RUN/'proposals'/name
        (folder/'source').mkdir(parents=True,exist_ok=False)
        counts={'mixed':'2,2,0,7,12,8','goose':'0,0,6,12,0,0','dairy':'5,0,0,12,0,0','wool':'0,4,0,12,0,0'}[case]
        labor=1 if case=='mixed' else 3
        (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::{name} {{
class Agent:public compiler_placement_sep08::AgentCore {{
public:
    Agent():AgentCore(compiler_labor_data::cold_farm({counts}),Support{{}},false,false,false,1,true,{labor},{mode}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
            'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../compiler/source/agent.cpp']},indent=2)+'\n')
        (folder/'README.md').write_text(f'# {name}\n\nIndependent cold {case} composition, placement mode{mode}, inherited labor mode{labor}. Exact lineage and controls in ../../LINEAGE.json. No promotion claim.\n')
        assert name not in catalog
        catalog[name]=str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
spec={'created_utc':datetime.now(timezone.utc).isoformat(),'parent':str(PARENT.relative_to(EXP)),
    'parent_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                    [PARENT/'compiler/source/agent.hpp',PARENT/'compiler/source/agent.cpp',PARENT/'cold_farm.hpp',PARENT/'ANALYSIS.json']},
    'variants':names,'discovery':{'seed_start':1000,'seeds':8,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'hypothesis':'Current distance scoring can expand land before filling owned tiles, delaying otherwise affordable animals. Prefer available already allocated quadrants as a controlled economic placement ablation.',
    'scope':'Four cold dated farms; unchanged service and hiring choice. Mode3 estimator sees the selected layout, so its distance-driven daily counts may legitimately differ. Not a promotion.'}
(RUN/'LINEAGE.json').write_text(json.dumps(spec,indent=2)+'\n')
(RUN/'run_discovery.py').write_bytes((PARENT/'run_discovery.py').read_bytes())
print('Prepared',len(names),'placement policies.')
