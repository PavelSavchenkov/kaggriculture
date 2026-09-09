"""Preserve the original animal policy and add one observed-state day repair."""
from datetime import datetime, timezone
from pathlib import Path
import csv
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
ORIGINAL = EXP / 'runs/animal_group_policy_sep08_001'
repair = ORIGINAL / 'weed38_repair_30s/h0'
results = list(csv.DictReader((repair.parent / 'RESULTS.csv').open()))
assert results[0]['extra_hires'] == '0' and results[0]['endpoint'] == '1'
assert results[0]['own_gain'] == '152' and results[0]['wheat_gain'] == '6'
source = RUN / 'source'
source.mkdir(exist_ok=False)
policy = (ORIGINAL / 'source/policy.hpp').read_text()
policy = policy.replace('#include "library.hpp"', '#include "repair.hpp"')
policy = policy.replace('namespace compositions::animal_groups_policy {',
    'namespace compositions::animal_repair {\nusing animal_groups_policy::library;')
policy = policy.replace('uint32_t matched_days=0,missed_days=0;',
    'uint32_t matched_days=0,missed_days=0,repaired_days=0;')
before = 'if(day_->matches(o))diagnostics_.matched_days|=uint32_t{1}<<o.day;'
after = '''// Only this cow course has an independently certified repair.
                if(!day_->matches(o) && family_==0 && choice_==2 && o.day==23){
                    const auto& repair=repair_day();
                    if(repair.matches(o)){day_=&repair;diagnostics_.repaired_days|=uint32_t{1}<<o.day;}
                }
                if(day_->matches(o))diagnostics_.matched_days|=uint32_t{1}<<o.day;'''
assert policy.count(before) == 1
policy = policy.replace(before, after)
policy = policy.replace('return {"animal_groups_policy"}', 'return {"animal_repair"}')
(source / 'policy.hpp').write_text(policy)
relative = str(ORIGINAL.relative_to(ROOT))
(source / 'repair.hpp').write_text(f'''#pragma once
#include "{relative}/source/library.hpp"
namespace compositions::animal_repair {{
const GuardedDay& repair_day();
}}
''')
values = [int(x) for path in (repair / 'guard.txt', repair / 'actions.txt') for x in path.read_text().split()]
assert values[0] == 23
(source / 'repair_data.inc').write_text('// Zero-additional-hire day23 repair; see LINEAGE.json.\nconstexpr int data[]={\n' +
    ',\n'.join(','.join(map(str, values[i:i+100])) for i in range(0, len(values), 100)) + '\n};\n')
(source / 'repair.cpp').write_text('''#include "repair.hpp"
namespace compositions::animal_repair {
namespace {
#include "repair_data.inc"
}
const GuardedDay& repair_day(){
    static const GuardedDay result=[] {
        GuardedDay g{};const int* p=data;g.plan.day=*p++;g.quadrants=*p++;
        for(int c=0;c<100;++c){g.check[c]=*p++;for(auto& x:g.tiles[c])x=*p++;}
        for(auto& x:g.shed)x=*p++;for(auto& x:g.seeds)x=*p++;
        for(auto& a:g.plan.actions){
            a.n_units=*p++;a.n_orders=*p++;
            for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            a.finalize();
        }
        if(p!=data+std::size(data) || g.plan.day!=23)std::abort();return g;
    }();return result;
}
}
''')
(source / 'opening.hpp').write_text('''#pragma once
#include "agents/common/api/agent_api.hpp"
namespace compositions::animal_repair {
template<class Parent,int Quantity> class Opening:public Parent {
public:
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        Parent::act(o,b,a);
        if(o.step==0){
            if(a.n_orders<2 || a.orders[0].op!=kag::M_BUY_PRODUCT || a.orders[1].op!=kag::M_SELL ||
                a.orders[0].item!=kag::WHEAT || a.orders[1].item!=kag::WHEAT)std::abort();
            a.orders[0].n=Quantity;a.orders[1].n=Quantity;a.finalize();
        }
    }
};
}
''')
registry_path = EXP / 'configs/league.json'
registry = json.loads(registry_path.read_text())
specs = [
    ('animal_repair_m1', 'animal_groups_m1', 1, False, False),
    ('animal_repair_m2', 'animal_groups_m2', 2, False, False),
    ('animal_repair_premium_m2', 'animal_premium_m2', 2, True, False),
    ('animal_repair_q24_premium_m2', 'animal_premium_m2', 2, True, True),
    ('premium_q24_s216', 'premium_sales_s216', 0, True, True),
]
for name, parent, mode, premium, q24 in specs:
    package = RUN / 'proposals' / name
    (package / 'source').mkdir(parents=True, exist_ok=False)
    includes = '#include "../../../source/policy.hpp"\n#include "../../../source/opening.hpp"\n'
    includes += '#include "experiments/v6/sep07_compositions_v0/runs/premium_sales_sep08_001/source/policy.hpp"\n'
    definition = f'class Base:public animal_repair::Policy {{ public: Base():Policy({mode}){{}} }};\n'
    base = 'Base'
    if premium:
        base = f'premium_sales::Policy<{base},216>'
    if q24:
        base = f'animal_repair::Opening<{base},24>'
    (package / 'source/agent.hpp').write_text(f'''#pragma once
{includes}namespace compositions::{name} {{
{definition}class Agent:public {base} {{
public:
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    original_package = ROOT / registry['animal_groups_m2']
    sources = [(original_package / p).resolve() for p in json.loads((original_package / 'agent.json').read_text())['sources']]
    sources = [source / 'repair.cpp', *sources]
    (package / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name,
        'header': 'source/agent.hpp', 'type': f'compositions::{name}::Agent',
        'sources': ['source/agent.cpp', *[os.path.relpath(p, package) for p in sources]]}, indent=2) + '\n')
    (package / 'README.md').write_text(f'''# {name}

Experimental descendant of {parent}. Animal mode {mode}; premium sale priority
{premium}; first-turn wheat quantity24 {q24}. Mode0 retains the accepted farm.

Modes1/2 preserve the original animal selector and its immutable day library.
When the cow-triple course's day23 guard fails, try the independently solved
zero-extra-worker weed38 repair only if its exact public/own starting contract
matches. It restores the crop and rejoins the original next-day program. Other
guard misses remain diagnosed and use the closest complete animal course.
Mode2 adapts sale timing using the selected repair's actual future orders.

All mutable state belongs to each instance and resets per episode. No seed,
future observation, live simulator or opponent private inventory is used.
Repair and inherited public components are attributed in ../../LINEAGE.json.
Operational, unchanged-case parity and full-league evaluation are required.
''')
    assert name not in registry
    registry[name] = str(package.relative_to(ROOT))
registry_path.write_text(json.dumps(registry, indent=2) + '\n')
inputs = [ORIGINAL / 'source/policy.hpp', ORIGINAL / 'source/library.hpp',
    ORIGINAL / 'LIBRARY_LINEAGE.json', repair / 'guard.txt', repair / 'actions.txt',
    repair / 'problem.json', repair.parent / 'RESULTS.csv',
    EXP / 'runs/premium_sales_sep08_001/source/policy.hpp',
    EXP / 'runs/opening_funding_sep08_001/source/policy.hpp']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'inputs': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
    'packages': specs, 'repair': results[0],
    'scope': 'Local day-solver repair and quantity24 composition of existing verified components. Original policy source remains unchanged. No source-world seed or future shops enter the runtime table.',
    'limits': 'One exact day23 repair, not general runtime scheduling. Full field and native validation pending.'}, indent=2) + '\n')
print('Prepared five C++ packages and one exact-state repair; original policy unchanged.')
