from datetime import datetime,timezone
from pathlib import Path
import hashlib,json,subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
old=EXP/'runs/cold_renewal_sep08_001'
all_plans=json.loads((old/'ESTIMATES.json').read_text())['proposals']
sources=[all_plans[i] for i in [0,50,98,157]]
with (RUN/'INPUT_PLANS.txt').open('x') as f:
    f.write('4\n')
    for p in sources:
        f.write(' '.join(map(str,[p['id'],p['herd'],p['crop'],p['cap'],p['hands'],len(p['lives'])]))+'\n')
        f.write(' '.join(map(str,p['daily_hands']))+'\n'+' '.join(map(str,p['quadrants']))+'\n')
        for l in p['lives']:f.write(' '.join(map(str,l))+'\n')
commands=[['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
    '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-I',str(ROOT),str(RUN/'search.cpp'),'-o',str(RUN/'search')],
    ['conda','run','-n','kaggriculture',str(RUN/'search'),str(RUN/'INPUT_PLANS.txt'),str(old/'scenarios.txt'),str(RUN/'ESTIMATES.json')]]
for i,command in enumerate(commands):
    result=subprocess.run(command,capture_output=True,text=True);(RUN/f'prepare_{i}.log').write_text(result.stdout+result.stderr);result.check_returncode()
(RUN/'PREPARE_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
data=json.loads((RUN/'ESTIMATES.json').read_text());names=[]
catalog=json.loads((EXP/'configs/league.json').read_text())
for p in data['plans']:
    if p['mode']==0:
        original=all_plans[p['base']]
        for k in ['lives','daily_hands','quadrants','produced']:assert p[k]==original[k],(p['base'],k)
        assert abs(p['cash']-original['cash'])<1e-6
    assert p['produced'][4]==72
    name=f"early_melon_b{p['base']}_m{p['mode']}";names.append(name)
    folder=RUN/'proposals'/name;(folder/'source').mkdir(parents=True,exist_ok=False)
    code='inline constexpr Life lives[]={\n'+''.join('{'+','.join(map(str,l))+'},\n' for l in p['lives'])+'};\n'
    code+='inline constexpr Support support={{'+','.join(map(str,p['daily_hands']))+'},{'+','.join(map(str,p['quadrants']))+'}};\n'
    (folder/'source/plan.inc').write_text(code)
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../../joint_day_routes_sep08_001/compiler/source/agent.hpp"
namespace compositions::{name} {{
#include "plan.inc"
class Agent:public joint_day_core::AgentCore {{public:
    Agent():AgentCore(std::vector<Life>(std::begin(lives),std::end(lives)),support,true,true,false,1,true,0,0,1) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent',
        'sources':['source/agent.cpp','../../../joint_day_routes_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
    (folder/'PLAN.json').write_text(json.dumps(p,indent=2)+'\n')
    (folder/'README.md').write_text(f"# {name}\n\nEarlier melon calendar on local cold-renewal base{p['base']}, mode{p['mode']}. Exact plan in PLAN.json. The unchanged compiler targets birth days; within-day timestamps are approximate proposal boundaries, not certified action times. No copied external route. Experimental, not promoted; exact replay idea origins and comparisons in ../../LINEAGE.json.\n")
    assert name not in catalog;catalog[name]=str(folder.relative_to(ROOT))
(EXP/'configs/league.json').write_text(json.dumps(catalog,indent=2)+'\n')
lineage={'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,'base_agents':[f"cold_renewal_p{i}" for i in [0,50,98,157]],
    'modes':{'0':'Exact old plan control.','1':'End melon cohort after day10; preserve old follow-up crops.','2':'End melon after day10 and regenerate gaps/crop renewals from day11.','3':'End melon at day10 hour12 and regenerate renewals including same-day starts. Compiler schedules by birth day, not exact hour.'},
    'ideas':['Top-player full-six melon harvest at age10 releases tiles sooner than our age12 calendar.',
        'Separate earlier harvest/sales from actual renewal composition changes with mode1.',
        'Test four farm families with the same unchanged compiler and workforce.'],
    'source_example':{'episode':106686702,'team_at_replay':'get some fries','seat':0,'tile':[1,3],'start_state':10,'end_state':246,'harvest':6,'origin_day':0},
    'origins':['research/crop_renewal_patterns_sep08_001/ANALYSIS.json','Local p362/cold-renewal temporal gap generator; no new donor schedule copied.'],
    'scope':'Discovery only; earlier modeled release is not an exact worker schedule. Must measure actual melon yield and follow-up births.',
    'source_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [RUN/'search.cpp',RUN/'prepare.py',old/'ESTIMATES.json',old/'scenarios.txt',
        EXP/'runs/joint_day_routes_sep08_001/compiler/source/agent.hpp',EXP/'runs/joint_day_routes_sep08_001/compiler/source/agent.cpp']}}
(RUN/'LINEAGE.json').write_text(json.dumps(lineage,indent=2)+'\n')
print('Prepared',len(names),'C++ plans; all4 controls and all72-melon-output predictions exact;',data['seconds'],'seconds.',flush=True)
