"""Isolate distribution of carried wheat after the day-solver delivery witness."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PARENT = EXP/'runs/compiler_care_sep08_001'
core=RUN/'compiler';(core/'source').mkdir(parents=True,exist_ok=False)
h=(PARENT/'compiler/source/agent.hpp').read_text().replace('compiler_care_sep08','compiler_feed_bundle_sep08')
h=h.replace('int care_mode=0)','int care_mode=0,int feed_bundle=4)')
h=h.replace('care_mode_(care_mode)','care_mode_(care_mode),feed_bundle_(feed_bundle)')
h=h.replace('int labor_mode_=0,placement_mode_=0,care_mode_=0;','int labor_mode_=0,placement_mode_=0,care_mode_=0,feed_bundle_=4;')
(core/'source/agent.hpp').write_text(h)
s=(PARENT/'compiler/source/agent.cpp').read_text().replace('compiler_care_sep08','compiler_feed_bundle_sep08')
old='const int bundle=is_animal(item)?1:4;';assert s.count(old)==1
s=s.replace(old,'const int bundle=is_animal(item)?1:item==WHEAT?feed_bundle_:4;')
s=s.replace('care_mode_>1)','care_mode_>1 || feed_bundle_<1 || feed_bundle_>4)')
(core/'source/agent.cpp').write_text(s)
(core/'agent.json').write_text(json.dumps({'format_version':1,'name':'compiler_feed_bundle_sep08_core','header':'source/agent.hpp',
    'type':'compositions::compiler_feed_bundle_sep08::Agent<>','sources':['source/agent.cpp']},indent=2)+'\n')
(core/'README.md').write_text('# Wheat pickup distribution\n\nOnly the wheat pickup bundle changes:1,2,or4. The earlier care correction, tile intentions, placement, workforce and all other priorities remain unchanged. Four reproduces the parent. Input counters still aggregate carried wheat; this test measures whether smaller loads distribute it better.\n')
catalog_path=EXP/'configs/league.json';catalog=json.loads(catalog_path.read_text());names=[]
for case in ['mixed','p355','p362','p4','p55']:
    for bundle in [1,2,4]:
        name=f'compiler_feed_{case}_b{bundle}';folder=RUN/'proposals'/name;(folder/'source').mkdir(parents=True,exist_ok=False)
        includes='#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"\n'
        if case=='mixed':
            args=f'compiler_labor_data::cold_farm(2,2,0,7,12,8),Support{{}},false,false,false,1,true,1,1,1,{bundle}'
        elif case in ['p355','p362']:
            parent=f'dated_expansion_{case}'
            includes+=f'#include "../../../../dated_expansion_sep08_001/proposals/{parent}/source/agent.hpp"\n'
            args=f'std::vector<Life>(std::begin({parent}::lives),std::end({parent}::lives)),{parent}::support,true,true,false,1,true,0,0,1,{bundle}'
        else:
            args=f'{case[1:]},true,true,true,1,true,0,0,1,{bundle}'
        (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../compiler/source/agent.hpp"
{includes}namespace compositions::{name} {{
class Agent:public compiler_feed_bundle_sep08::AgentCore {{
public:
    Agent():AgentCore({args}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
            'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../compiler/source/agent.cpp']},indent=2)+'\n')
        (folder/'README.md').write_text(f'# {name}\n\nFixed {case} farm, wheat pickup bundle{bundle}. Only delivery distribution changes from the corrected compiler. Small and large cold cases use local dated plans; p4/p55 retain exact earlier replay lineage, placement, service masks and workforce. Four is an unchanged control. See ../../LINEAGE.json. Experimental, not promoted.\n')
        assert name not in catalog;catalog[name]=str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
spec={'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,
    'discovery':{'seed_start':1000,'seeds':8,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'parent':'runs/compiler_care_sep08_001','evidence':'runs/dated_day_service_sep08_002/RESULTS.json',
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                   [PARENT/'compiler/source/agent.hpp',PARENT/'compiler/source/agent.cpp',RUN/'prepare.py']},
    'hypothesis':'Enough carried wheat can be stranded on a few workers while other workers care for unfed animals. Smaller pickup bundles may spread the existing wheat among routes.',
    'scope':'Single physical input-distribution choice across five fixed farms. No shop selector, global work-priority change, or incumbent mutation.'}
(RUN/'LINEAGE.json').write_text(json.dumps(spec,indent=2)+'\n')
(RUN/'run_discovery.py').write_bytes((PARENT/'run_discovery.py').read_bytes())
print('Prepared',len(names),'wheat distribution policies.')
