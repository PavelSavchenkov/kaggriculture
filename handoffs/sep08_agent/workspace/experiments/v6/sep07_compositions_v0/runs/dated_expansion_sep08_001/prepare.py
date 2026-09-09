"""Estimate larger dated farms, preserve family diversity, compile finalists."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
source = EXP/'runs/compiler_care_sep08_001/discovery/compiler_care_mixed_m1_vs_public_router.json'
games = json.loads(source.read_text())['games']
with (RUN/'scenarios.txt').open('w') as f:
    f.write(f'two_sided_v1 {len(games)}\n')
    for g in games:
        profile = g['opponent_profile']
        fixed = sum(row[1] for row in profile['fixed_costs'])
        f.write(f"{g['seed']} {g['seat']} {g['cash']} 0 0 {len(profile['flows'])} {fixed}\n")
        f.write(' '.join(map(str,g['shops']))+'\n')
        for row in profile['flows']:
            f.write(' '.join(map(str,row))+'\n')
commands = [['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-I',str(ROOT),str(RUN/'search.cpp'),'-o',str(RUN/'search')],
    ['conda','run','-n','kaggriculture',str(RUN/'search'),str(RUN/'scenarios.txt'),str(RUN/'estimated')]]
for i,command in enumerate(commands):
    result = subprocess.run(command,capture_output=True,text=True)
    (RUN/f'prepare{i}.log').write_text(result.stdout+result.stderr)
    result.check_returncode()
    if result.stdout:
        print(result.stdout,flush=True)
(RUN/'PREPARE_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
proposals = json.loads((RUN/'estimated/proposals.json').read_text())
catalog_path = EXP/'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for p in proposals:
    if not p['selected']:
        continue
    name = f"dated_expansion_p{p['id']}"
    folder = RUN/'proposals'/name
    (folder/'source').mkdir(parents=True,exist_ok=False)
    plan = 'inline constexpr Life lives[]={\n'+''.join('{'+','.join(map(str,l))+'},\n' for l in p['lives'])+'};\n'
    plan += 'inline constexpr Support support={{'+','.join(map(str,p['daily_hands']))+'},{'+','.join(map(str,p['quadrants']))+'}};\n'
    (folder/'source/plan.inc').write_text(plan)
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../../compiler_care_sep08_001/compiler/source/agent.hpp"
namespace compositions::{name} {{
#include "plan.inc"
class Agent:public compiler_care_sep08::AgentCore {{
public:
    Agent():AgentCore(std::vector<Life>(std::begin(lives),std::end(lives)),support,true,true,false,1,true,0,0,1) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../../compiler_care_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
    (folder/'PLAN.json').write_text(json.dumps(p,indent=2)+'\n')
    (folder/'README.md').write_text(f"# {name}\n\nCold dated farm, family {p['family']}, expansion day{p['day']}, count{p['count']}, post-expansion hands{p['hands']}. Exact dated lives, shared explicit placement and support in PLAN.json. Productive reactive service uses the care-bank correction. Local rule-built composition and compiler; no copied donor route. Experimental estimate-selected candidate, not promoted. Lineage in ../../LINEAGE.json.\n")
    assert name not in catalog
    catalog[name] = str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
spec = {'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,
    'baseline':'compiler_care_mixed_m1','incumbent':'observed_sale_lead_start_216',
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
        [source,RUN/'search.cpp',RUN/'prepare.py',EXP/'include/estimate.hpp',EXP/'include/biology.hpp',EXP/'include/economics.hpp',
         EXP/'runs/compiler_care_sep08_001/compiler/source/agent.hpp',EXP/'runs/compiler_care_sep08_001/compiler/source/agent.cpp']},
    'discovery':{'seed_start':1000,'seeds':4,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'ideas':['The small cold farm nearly reaches its biological ceiling; enlarge dated animals and crops around funding milestones.',
             'Use the identical explicit owned-first tile layout and daily workforce in estimate and executable policy.',
             'Select top economics plus one finalist per family and workforce, preserving exploration despite uncalibrated work/finance penalties.'],
    'lineage':['Initial cold2cow2sheep/7wheat/12melon/8strawberry from local compiler_labor_sep08_001 factory.',
               'Owned-first placement from compiler_placement_sep08_001 and full-bank correction from compiler_care_sep08_001.',
               'Larger herd/crop milestones motivated by fresh top-player profiles; no source schedule or exact donor composition copied.'],
    'limits':['Estimation conditions on fixed public_router flows observed against the prior small farm; actual opponent responses can change.',
              'Productive biology assumes more regular service than reactive execution. Fixed labor may leave work uncovered.',
              'Negative estimated cash is an explicit infeasibility warning; it is not granted credit. Exact engine enforces cash.',
              'Land is requested on each explicit birth day, changing early-purchase timing from the previous reactive control.',
              'Discovery seeds are already exposed. No frozen promotion or current-shop online selector is claimed.']}
(RUN/'LINEAGE.json').write_text(json.dumps(spec,indent=2)+'\n')
print('Prepared',len(names),'dated expansion packages.')
