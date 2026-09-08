"""Isolate care-bank timing from all existing composition/compiler choices."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PARENT = EXP/'runs/compiler_placement_sep08_001'


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


core = RUN/'compiler'
(core/'source').mkdir(parents=True, exist_ok=False)
header = (PARENT/'compiler/source/agent.hpp').read_text().replace('compiler_placement_sep08', 'compiler_care_sep08')
header = header.replace('int placement_mode=0)', 'int placement_mode=0,int care_mode=0)')
header = header.replace('placement_mode_(placement_mode)', 'placement_mode_(placement_mode),care_mode_(care_mode)')
header = replace(header, 'int labor_mode_=0,placement_mode_=0;', 'int labor_mode_=0,placement_mode_=0,care_mode_=0;')
(core/'source/agent.hpp').write_text(header)
source = (PARENT/'compiler/source/agent.cpp').read_text().replace('compiler_placement_sep08', 'compiler_care_sep08')
source = replace(source, 'placement_mode_>1)', 'placement_mode_>1 || care_mode_<0 || care_mode_>1)')
source = replace(source, 't.pending_care_bonus<rule.max_held-1 &&',
                 '(t.pending_care_bonus<rule.max_held-1 || (care_mode_ && produces(t.what,t.planted_day,o.day+1))) &&')
(core/'source/agent.cpp').write_text(source)
(core/'agent.json').write_text(json.dumps({'format_version':1,'name':'compiler_care_sep08_core','header':'source/agent.hpp',
    'type':'compositions::compiler_care_sep08::Agent<>','sources':['source/agent.cpp']}, indent=2)+'\n')
(core/'README.md').write_text('# Care-bank timing comparison\n\nMode0 preserves the placement compiler. Mode1 permits CARE at a full old bonus bank if production resets that bank tonight and a later production can use the new bonus. Source service masks, other priorities, hiring, and placement are unchanged.\n')
catalog_path = EXP/'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for case in ['mixed','goose','p4','p55','p4_productive','p55_productive']:
    for mode in range(2):
        name = f'compiler_care_{case}_m{mode}'
        folder = RUN/'proposals'/name
        (folder/'source').mkdir(parents=True, exist_ok=False)
        if case in ['mixed','goose']:
            counts = '2,2,0,7,12,8' if case == 'mixed' else '0,0,6,12,0,0'
            labor = 1 if case == 'mixed' else 3
            args = f'compiler_labor_data::cold_farm({counts}),Support{{}},false,false,false,1,true,{labor},1,{mode}'
        else:
            program = 4 if case.startswith('p4') else 55
            service = 'false' if case.endswith('productive') else 'true'
            args = f'{program},true,true,{service},1,true,0,0,{mode}'
        (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"
namespace compositions::{name} {{
class Agent:public compiler_care_sep08::AgentCore {{
public:
    Agent():AgentCore({args}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
            'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../compiler/source/agent.cpp']}, indent=2)+'\n')
        (folder/'README.md').write_text(f'# {name}\n\nCare timing mode{mode}, {case} fixed composition. Dense source cases retain recorded placement and workforce; productive variants omit source service masks. Cold cases use owned-first placement and prior workforce. Exact lineage in ../../LINEAGE.json. Experimental compiler; no promotion claim.\n')
        assert name not in catalog
        catalog[name] = str(folder.relative_to(ROOT))
        names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2)+'\n')
spec = {'created_utc':datetime.now(timezone.utc).isoformat(),'parent':str(PARENT.relative_to(EXP)),
    'parent_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                    [PARENT/'compiler/source/agent.hpp',PARENT/'compiler/source/agent.cpp']},
    'variants':names,'discovery':{'seed_start':1000,'seeds':8,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'witness':'runs/cold_later_days_sep08_001/CARE_BANK_WITNESS.json',
    'hypothesis':'Production tonight clears the old care bank before banking today CARE. The old cap check suppresses useful CARE on the first full-bank production eve.',
    'scope':'One-condition correction with disabled controls. Fixed biology/routing comparison on cold and dense recorded farms. Does not modify the strong incumbent.'}
(RUN/'LINEAGE.json').write_text(json.dumps(spec, indent=2)+'\n')
(RUN/'run_discovery.py').write_bytes((PARENT/'run_discovery.py').read_bytes())
print('Prepared', len(names), 'care-timing policies.')
