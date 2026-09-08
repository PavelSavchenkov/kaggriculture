"""Expose three V25 execution components without changing its production tapes."""
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PARENT = EXP / 'runs/ahmed_v25_sep08_001/proposals/ahmed_v25'
source = RUN / 'source'
source.mkdir(exist_ok=False)
header = (PARENT / 'source/agent.hpp').read_text().replace('compositions::ahmed_v25', 'compositions::v25_components')
header = header.replace('    int route_=0', '    const int mask_;\n    int route_=0')
header = header.replace('public:\n', 'public:\n    explicit Agent(int mask=7):mask_(mask){}\n')
(source / 'policy.hpp').write_text(header)
code = (PARENT / 'source/agent.cpp').read_text()
code = code.replace('"agent.hpp"', '"policy.hpp"').replace('namespace compositions::ahmed_v25', 'namespace compositions::v25_components')
code = code.replace('#include "data.inc"', f'#include "{PARENT.relative_to(ROOT)}/source/data.inc"')
needle = '    for(int u=0;u<a.n_units;++u){\n        int x='
assert code.count(needle) == 1
code = code.replace(needle, '    if(mask_&1)for(int u=0;u<a.n_units;++u){\n        int x=')
code = code.replace('if(o.step+1<=718 &&', 'if((mask_&2) && o.step+1<=718 &&')
code = code.replace('if(o.step>=718){', 'if((mask_&4) && o.step>=718){')
(source / 'policy.cpp').write_text(code)
registry_path = EXP / 'configs/league.json'
registry = json.loads(registry_path.read_text())
for mask in range(8):
    name = f'v25_components_m{mask}'
    package = RUN / 'proposals' / name
    (package / 'source').mkdir(parents=True, exist_ok=False)
    (package / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
namespace compositions::{name} {{
class Agent:public v25_components::Agent {{
public:
    Agent():v25_components::Agent({mask}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (package / 'agent.json').write_text(json.dumps({'format_version':1, 'name':name,
        'header':'source/agent.hpp', 'type':f'compositions::{name}::Agent',
        'sources':['source/agent.cpp','../../source/policy.cpp']}, indent=2)+'\n')
    (package / 'README.md').write_text(f'''# {name}

Controlled ablation of the verified Ahmed V25 C++ port. Bit1 enables weed repair,
bit2 sale leading, bit4 final-step deposits/liquidation. Mask{mask}; mask7 must
reproduce the unmodified source port exactly. All four Yusuke tapes and observed
branch thresholds stay fixed. Episode state is per instance; the component mask
is immutable. Only the normal own/public observation API is used.

External Apache2.0 lineage is inherited from Ahmed V25 and its cited donors;
see ../../LINEAGE.json and its parent agent's LICENSE.txt. Rating is unknown
for this variant. No competitive or source-equivalence claim until tested.
''')
    assert name not in registry
    registry[name] = str(package.relative_to(ROOT))
registry_path.write_text(json.dumps(registry, indent=2)+'\n')
(RUN / 'LINEAGE.json').write_text(json.dumps({'parent':'ahmed_v25',
    'original_lineage':'runs/ahmed_v25_sep08_001/LINEAGE.json',
    'original_source_parity_actions':10793,
    'inputs':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in (PARENT/'source/agent.hpp',PARENT/'source/agent.cpp',PARENT/'source/data.inc',PARENT/'LICENSE.txt')},
    'change':'Three immutable instance flags gate the existing weed, sale-leading and terminal blocks. No method or tape edits.',
    'objective':'Separate production restored by weed handling, revenue timing, and terminal delivery. Transfer only useful components into the main agent; the donor whole agent is weaker than current q24 on the neutral panel.',
    'status':'Packages prepared; control equivalence and operational checks pending.'}, indent=2)+'\n')
print('Prepared8 V25 component masks; full-mask equivalence must be verified.')
