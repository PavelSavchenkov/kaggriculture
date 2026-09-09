"""Export reviewed Salem tape and four C++ component ablations."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import sys

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
sys.path.insert(0,str(EXP/'scripts'))
from generate_public_routes import triple

upstream=EXP/'research/refresh_sep08_1008/notebook_audit/salem'
actions=json.loads((upstream/'actions.json').read_text())
assert len(actions)==720
offsets=[];values=[]
for action in actions:
    units=[triple(u or ['PASS']) for u in [action.get('farmer') or ['PASS'],*action.get('hands',[])]]
    orders=[triple(o,True) for o in action.get('market',[])]
    assert len(orders)<=10
    offsets.append(len(values));values += [len(units),len(orders),*(n for v in units+orders for n in v)]
data='// Literal upstream tape; see LINEAGE.json.\n'
data+='constexpr int offsets[]={'+','.join(map(str,offsets))+'};\n'
data+='constexpr int values[]={\n'+',\n'.join(','.join(map(str,values[i:i+100])) for i in range(0,len(values),100))+'\n};\n'
with (RUN/'source/data.inc').open('x') as f:f.write(data)
(RUN/'source/policy.cpp').write_text('''#include "policy.hpp"
namespace compositions::salem_port {
namespace {
#include "data.inc"
struct Tables {
    std::array<kag::Action,720> actions{};
    Tables(){
        for(int step=0;step<720;++step){
            auto& a=actions[step];const int* p=values+offsets[step];a.n_units=*p++;a.n_orders=*p++;
            for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            for(int j=0;j<a.n_orders;++j){a.orders[j]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            a.finalize();
        }
    }
};
}
const kag::Action& planned(int step){static const Tables data;return data.actions[std::clamp(step,0,719)];}
}
''')
registry_path=EXP/'configs/league.json';registry=json.loads(registry_path.read_text())
for mode in range(4):
    name=f'salem_sep08_m{mode}';folder=RUN/'proposals'/name
    (folder/'source').mkdir(parents=True,exist_ok=False)
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
namespace compositions::{name} {{
class Agent:public salem_port::Policy {{
public:
    Agent():Policy({mode}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../source/policy.cpp']},indent=2)+'\n')
    (folder/'README.md').write_text(f'''# {name}

Experimental C++ port of Salem Ali's Sep8 public fixed8cow/4sheep course.
Mode{mode}: bit1 enables the upstream per-worker weed replay; bit2 enables
conditional next-turn sales with duplicate repayment. Mode3 is the full port.
Immutable shared tape; mutable repairs and pending sales are per instance/reset.
Source parity, operational checks and local playing strength remain unverified.
Exact artifact hashes and upstream claims are in ../../LINEAGE.json. Rating for
this artifact and original replay provenance are unknown. No independent code
license is supplied in its downloaded metadata; retain source attribution.
''')
    assert name not in registry;registry[name]=str(folder.relative_to(ROOT))
registry_path.write_text(json.dumps(registry,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'source':'https://www.kaggle.com/code/salemali7/kaggriculture-2900',
    'source_file':str((upstream/'reference.py.txt').relative_to(EXP)),
    'source_sha256':hashlib.sha256((upstream/'reference.py.txt').read_bytes()).hexdigest(),
    'tape_sha256':hashlib.sha256((upstream/'actions.json').read_bytes()).hexdigest(),
    'ideas':['Fixed8cow/4sheep farm reaches its intended herd by step192.',
        'Repair weed-blocked planting/pastures with DIG, intended action, then8shifted tape actions.',
        'Advance next-turn milk/wool/berry/melon sales when current demand is zero; reserve own pickups and current sales, then suppress duplicates.'],
    'changes':['Typed C++ tape export.','Per-instance state replaces Python seat-keyed globals.',
        'Local API worker-count alignment and canonical ignored action arguments.','Four component ablations.'],
    'validation':'Prepared only; source action parity, required operations and league screen pending.',
    'lineage_limits':'Original replay episode provenance is absent. Notebook rating and local tables are not independently verified.'},indent=2)+'\n')
print('Prepared four Salem C++ ablations; full source mode3. Validation pending.')
