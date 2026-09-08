"""Preserve the exact port and expose independent causal layer controls."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
original = EXP / 'league/ahmed_v23/source'
source = RUN / 'source'
source.mkdir(exist_ok=True)
header = (original / 'agent.hpp').read_text().replace('namespace kag::agents::ahmed_v23', 'namespace compositions::ahmed_components')
header = header.replace('    struct Pending', '    unsigned disabled_=0;\n    struct Pending')
header = header.replace('public:\n    static', 'public:\n    explicit Agent(unsigned disabled=0):disabled_(disabled){}\n    static')
(source / 'core.hpp').write_text(header)
code = (original / 'agent.cpp').read_text().replace('#include "agent.hpp"', '#include "core.hpp"')
code = code.replace('namespace kag::agents::ahmed_v23 {', 'namespace compositions::ahmed_components {\nusing namespace kag;')
code = code.replace('#include "data.inc"', '#include "../../../league/ahmed_v23/source/data.inc"')
code = code.replace('    for(int u=0;u<a.n_units;++u){\n        int x=', '    if(!(disabled_&1))for(int u=0;u<a.n_units;++u){\n        int x=')
code = code.replace('    if(due_==o.step)', '    if(!(disabled_&2) && due_==o.step)')
code = code.replace('    if(o.step+1<=718', '    if(!(disabled_&2) && o.step+1<=718')
code = code.replace('    budget_guard(o,route_,a);room_guard(o,route_,a);', '    if(!(disabled_&4))budget_guard(o,route_,a);\n    if(!(disabled_&8))room_guard(o,route_,a);')
code = code.replace('    for(int i=0;i<a.n_orders;++i){auto& m=a.orders[i];if(m.op==M_SELL){m.n=std::clamp', '    if(!(disabled_&16))for(int i=0;i<a.n_orders;++i){auto& m=a.orders[i];if(m.op==M_SELL){m.n=std::clamp')
code = code.replace('    for(int i=0;i<n && a.n_orders<10;++i)', '    if(!(disabled_&32))for(int i=0;i<n && a.n_orders<10;++i)')
code = code.replace('    if(o.step>=718){', '    if(!(disabled_&64) && o.step>=718){')
assert all(f'disabled_&{bit}' in code for bit in [1,2,4,8,16,32,64])
(source / 'core.cpp').write_text(code)
variants = {'ahmed_layers_all': 0, 'ahmed_no_weed': 1, 'ahmed_no_lead': 2, 'ahmed_no_budget': 4,
            'ahmed_no_room': 8, 'ahmed_no_clamp': 16, 'ahmed_no_dead': 32, 'ahmed_no_terminal': 64,
            'ahmed_tape_only': 127}
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
for name, mask in variants.items():
    target = RUN / 'proposals' / name
    (target / 'source').mkdir(parents=True, exist_ok=False)
    (target / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/core.hpp"
namespace kag::agents::{name} {{
class Agent:public compositions::ahmed_components::Agent {{
public:
    Agent():compositions::ahmed_components::Agent({mask}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (target / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (target / 'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'kag::agents::{name}::Agent','sources':['source/agent.cpp','../../source/core.cpp']},indent=2)+'\n')
    (target / 'README.md').write_text(f'# {name}\n\nAhmedV23 causal component control; disabled layer mask={mask}. Complete source lineage is in ../../LINEAGE.json. Full-record parity and discovery screen pending; no promotion.\n')
    assert name not in catalog
    catalog[name] = str(target.relative_to(ROOT))
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
report={'created_utc':datetime.now(timezone.utc).isoformat(),'parent':'ahmed_v23',
        'source_lineage':'league/ahmed_v23/IMPORT.json', 'scope':'Single-layer ablations of the exact port; mask0 must match all parent records.',
        'source_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in original.iterdir() if p.is_file()},
        'variants':variants,'local_changes':['Independent source namespace','Per-instance immutable disable mask; no changes to enabled layer semantics'],
        'limitations':['Recorded-route compositions remain borrowed from ThomasV5/2.','No Kaggle score inference from discovery.']}
(RUN/'LINEAGE.json').write_text(json.dumps(report,indent=2)+'\n')
print('Created',len(variants),'independent layer controls; original exact port unchanged.')
