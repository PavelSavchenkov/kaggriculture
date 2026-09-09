"""Export four baseline contexts and the twelve independently audited courses."""
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime,timezone
from pathlib import Path
import hashlib
import json
import os
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
prior=EXP/'runs/animal_groups_sep08_001'
out=RUN/'baselines';out.mkdir(exist_ok=False)
contexts=[('cow0','public',1014),('cow1','public',1019),('sheep0','public',1016),('sheep1','pass',1048)]
def export(spec):
    name,opponent,seed=spec
    cmd=['conda','run','-n','kaggriculture',str(ROOT/'day_solver/with_runtime.sh'),
        str(RUN/f'build/export_{opponent}'),str(seed),str(out/name)]
    with (out/(name+'.log')).open('x') as log:subprocess.run(cmd,stdout=log,stderr=subprocess.STDOUT,check=True)
    return cmd
with ThreadPoolExecutor(max_workers=2) as pool:commands=list(pool.map(export,contexts))
for folder in (prior/'integrated_audit',RUN/'contexts_audit'):
    execution=json.loads((folder/'EXECUTION.json').read_text())
    assert execution['inputs_unchanged'] and all(c['returncode']==0 for c in execution['cases'])
families=[['baseline','cow_9','cow_triple'],['baseline','sheep_3','sheep_12','sheep_13','sheep_triple']]
values=[];paths=[];fixtures=[]
for family,names in enumerate(families):
    first=15 if family==0 else 20
    values.extend([first,len(names)])
    for choice,name in enumerate(names):
        for leaf in range(2):
            if not choice:folder=out/(('cow' if family==0 else 'sheep')+str(leaf))
            elif leaf==0:folder=prior/'integrated_audit'/name
            else:folder=RUN/'contexts_audit'/(name+('_alternate' if family==0 else '_berry'))
            if choice:
                expected=json.loads((folder/'MATCHED_RESULT.json').read_text())
                fixtures.append({'family':family,'choice':choice,'leaf':leaf,'seed':expected['seed'],
                    'opponent':'pass' if family==1 and leaf else 'public','cash':expected['cash'],
                    'rival_cash':expected['rival_cash'],'reference':str(folder.relative_to(EXP))})
            for day in range(first,30):
                g=folder/f'days/{day}/guard.txt';a=folder/f'days/{day}/actions.txt'
                guard=[int(x) for x in g.read_text().split()];actions=[int(x) for x in a.read_text().split()]
                assert guard[0]==day and len(guard)==2+1300+12+5
                paths.extend([g,a]);values.extend(guard);values.extend(actions)
code='// Audited farm programs and baseline intentions; see LIBRARY_LINEAGE.json.\nconstexpr int data[]={\n'
code+=',\n'.join(','.join(map(str,values[i:i+120])) for i in range(0,len(values),120))+'\n};\n'
(RUN/'source/data.inc').write_text(code)
(RUN/'LIBRARY_FIXTURES.json').write_text(json.dumps(fixtures,indent=2)+'\n')
(RUN/'LIBRARY_FIXTURES.txt').write_text(''.join(f"{f['family']} {f['choice']} {f['leaf']} {f['seed']} {f['opponent']} {f['cash']} {f['rival_cash']}\n" for f in fixtures))
(RUN/'LIBRARY_LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'baseline_export_commands':commands,'source_files':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
    'integers':len(values),'families':families,'days':190,
    'scope':'Immutable intended own programs; no source seed, source-world prices or future shop observations exported into policy tables.',
    'economics':'Daily intended product flows and fixed animal/seed/hire costs derived from actual compiled market orders. Conditional observed-shop scenarios drive runtime choice.',
    'limits':'Cow future weed cost uses equal mixture of two solved cases. General labor prediction, other entry herds/dates and rival crop expansion remain incomplete.'},indent=2)+'\n')
registry_path=EXP/'configs/league.json';registry=json.loads(registry_path.read_text());parent=ROOT/registry['empty_sale_slots_m2']
parent_sources=[(parent/p).resolve() for p in json.loads((parent/'agent.json').read_text())['sources']]
specs=[('animal_groups_m0',0,-1,-1),('animal_groups_m1',1,-1,-1),('animal_groups_m2',2,-1,-1),
    ('animal_groups_cow1',1,0,1),('animal_groups_cow3',1,0,2),('animal_groups_sheep12',1,1,2),('animal_groups_sheep3',1,1,4)]
for name,mode,family,choice in specs:
    package=RUN/'proposals'/name;(package/'source').mkdir(parents=True,exist_ok=False)
    (package/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
namespace compositions::{name} {{
class Agent:public animal_groups_policy::Policy {{
public:
    Agent():Policy({mode},{family},{choice}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (package/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (package/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent',
        'sources':['source/agent.cpp','../../source/library.cpp',*[os.path.relpath(p,package) for p in parent_sources]]},indent=2)+'\n')
    (package/'README.md').write_text(f'''# {name}

Experimental strongest-parent animal-course policy. Mode {mode}, forced family
{family}, forced choice {choice}; -1 means observed-state economic selection.
Mode 0 is the unchanged parent; mode 1 uses compiled market orders; mode 2
adds live inventory-based sale anticipation and removes empty sale orders.

Choose crops versus one/three cows on day 15 in the imported wool farm, or
crops versus sheep on three eligible tiles on day 20 in the crop farm. Preserve
the observed strawberry branch and select the cow weed schedule from current
farm state. Price full courses across 32 conditional future shop scenarios,
charging compiled labor and other fixed costs. Prior general animal selection
and other parent behavior remain active before a new course is selected.

After investment, a failed daily guard records a diagnostic and continues the
closest complete course instead of reverting to the old herd. This is not yet
validated as a general repair. Static tables are immutable; all decisions and
diagnostics are per instance and reset. Only public/own observations are used.
Original animal compiler and donor lineage are in LIBRARY_LINEAGE.json and
the referenced parent run. Required operations and league validation pending.
''')
    assert name not in registry;registry[name]=str(package.relative_to(ROOT))
registry_path.write_text(json.dumps(registry,indent=2)+'\n')
print('Exported',len(values),'integers, twelve courses, seven policy packages')
