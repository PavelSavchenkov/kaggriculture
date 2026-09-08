"""Compile certified cheaper cow courses into an observation-driven C++ policy."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PARENT = EXP / 'runs/animal_repair_sep08_001'
GROUP = EXP / 'runs/animal_group_policy_sep08_001'
COURSES = EXP / 'runs/animal_service_policy_sep08_001/retained_parent'
assert json.loads((COURSES / 'RESULTS.json').read_text())['complete']
source = RUN / 'source'
source.mkdir(exist_ok=False)
inputs = [PARENT / 'source/policy.hpp', GROUP / 'source/library.hpp', COURSES / 'RESULTS.json']
values = []
for seed in (1014, 1019):
    for day in range(15, 30):
        for filename in ('guard.txt', 'actions.txt'):
            path = COURSES / str(seed) / 'days' / str(day) / filename
            inputs.append(path)
            data = [int(x) for x in path.read_text().split()]
            if filename == 'guard.txt':
                assert data[0] == day and len(data) == 1319
            values.extend(data)
(source / 'data.inc').write_text('// Certified cow courses; see LINEAGE.json.\nconstexpr int data[]={\n' +
    ',\n'.join(','.join(map(str, values[i:i+120])) for i in range(0, len(values), 120)) + '\n};\n')
(source / 'library.hpp').write_text('''#pragma once
#include "experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/source/library.hpp"
namespace compositions::cow_service_retained {
const animal_groups_policy::Library& library();
}
''')
(source / 'library.cpp').write_text('''#include "library.hpp"
namespace compositions::cow_service_retained {
namespace {
#include "data.inc"
}
const animal_groups_policy::Library& library(){
    static const auto result=[] {
        auto result=animal_groups_policy::library();const int* p=data;
        for(int leaf=0;leaf<2;++leaf){
            auto& course=result[0].choices[2][leaf];course={};course.days.reserve(15);
            for(int day=15;day<30;++day){
                GuardedDay g{};g.plan.day=*p++;g.quadrants=*p++;
                for(int cell=0;cell<100;++cell){g.check[cell]=*p++;for(auto& x:g.tiles[cell])x=*p++;}
                for(auto& x:g.shed)x=*p++;for(auto& x:g.seeds)x=*p++;
                for(auto& a:g.plan.actions){
                    a.n_units=*p++;a.n_orders=*p++;
                    for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
                    for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
                    a.finalize();
                }
                if(g.plan.day!=day)std::abort();course.days.push_back(g);
            }
            animal_groups_policy::prepare(course);
        }
        if(p!=data+std::size(data))std::abort();return result;
    }();return result;
}
}
''')
policy = (PARENT / 'source/policy.hpp').read_text()
policy = policy.replace('#include "repair.hpp"', '#include "library.hpp"\n#include "experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/source/repair.hpp"')
policy = policy.replace('namespace compositions::animal_repair {\nusing animal_groups_policy::library;',
    'namespace compositions::cow_service_retained {\nusing animal_repair::repair_day;')
policy = policy.replace('    int mode_,forced_family_', '    bool repriced_;\n    int mode_,forced_family_')
policy = policy.replace('const auto& family=library()[f];', 'const auto& family=(repriced_?library():animal_groups_policy::library())[f];')
policy = policy.replace('int locked_leaf=-1):mode_(mode)', 'int locked_leaf=-1,bool repriced=true):repriced_(repriced),mode_(mode)')
assert 'bool repriced=true' in policy and 'using animal_groups_policy::library' not in policy
policy = policy.replace('return {"animal_repair"}', 'return {"cow_service_retained"}')
(source / 'policy.hpp').write_text(policy)
registry_path = EXP / 'configs/league.json'
registry = json.loads(registry_path.read_text())
parent = ROOT / registry['animal_repair_q24_premium_m2']
parent_sources = [(parent / p).resolve() for p in json.loads((parent / 'agent.json').read_text())['sources']]
for name, repriced in [('cow_service_retained_q24_premium_m2', True), ('cow_service_retained_fixed_choices_m2', False)]:
    package = RUN / 'proposals' / name
    (package / 'source').mkdir(parents=True, exist_ok=False)
    (package / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/source/opening.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/premium_sales_sep08_001/source/policy.hpp"
namespace compositions::{name} {{
class Base:public cow_service_retained::Policy {{
public: Base():Policy(2,-1,-1,-1,{str(repriced).lower()}){{}}
}};
class Agent:public animal_repair::Opening<premium_sales::Policy<Base,216>,24> {{
public: static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    sources = [source / 'library.cpp', *parent_sources]
    (package / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name,
        'header': 'source/agent.hpp', 'type': f'compositions::{name}::Agent',
        'sources': ['source/agent.cpp', *[os.path.relpath(p, package) for p in sources]]}, indent=2) + '\n')
    (package / 'README.md').write_text(f'''# {name}

Descendant of submitted animal_repair_q24_premium_m2. Replaces both complete
day-15 three-cow continuations with independently certified schedules that
omit two capped early cares, retain cheaper original schedules, and save four hires, $411 per source context.
Reprice the selector with the new labor bill: {repriced}. The false variant
retains original selection values to isolate execution from changed choices.

External farm and controller lineage remains inherited; exact sources and
local day-solver changes are in ../../LINEAGE.json. All mutable state is per
instance. Runtime uses only own/public observations; no seed, hidden rival
inventory or future shops. Existing guarded weed repair remains available.
No whole-league gain or general repair claim until evaluations complete.
''')
    assert name not in registry
    registry[name] = str(package.relative_to(ROOT))
registry_path.write_text(json.dumps(registry, indent=2) + '\n')
fixtures = json.loads((GROUP / 'LIBRARY_FIXTURES.json').read_text())
for fixture in fixtures:
    if fixture['family'] == 0 and fixture['choice'] == 2:
        audit = COURSES / f"{fixture['seed']}_audit/MATCHED_RESULT.json"
        data = json.loads(audit.read_text())
        assert data['cash'] == fixture['cash'] + 411 and data['rival_cash'] == fixture['rival_cash']
        fixture.update(cash=data['cash'], reference=str(audit.relative_to(EXP)))
(RUN / 'FIXTURES.json').write_text(json.dumps(fixtures, indent=2) + '\n')
(RUN / 'FIXTURES.txt').write_text(''.join(f"{f['family']} {f['choice']} {f['leaf']} {f['seed']} {f['opponent']} {f['cash']} {f['rival_cash']}\n" for f in fixtures))
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'parent': 'animal_repair_q24_premium_m2', 'parent_submission': 56101451,
    'inputs': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
    'change': 'Replace all 15 days in each cow-triple leaf, preserving reduced care-bank entry states and recalculating intended flow/fixed cost from executable orders.',
    'source_evidence': 'Two certified 719-turn source worlds each save411 cash, equal production and rival cash. General policy results pending.',
    'scope': 'Immutable own programs only. Source seeds identify offline provenance and fixtures; they never enter runtime selection.',
    'previous_goal_turn': 'Progress: completed requested 13-page PDF and all-page visual verification.'}, indent=2) + '\n')
print('Prepared two cow-service policies and 12 complete-course fixtures.')
