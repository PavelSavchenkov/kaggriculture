from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
games = []
sources = []
for opponent in ['empty_sale_slots_m2','public_router']:
    source = EXP/'runs/day_service_bank_sep08_001/discovery'/f'service_bank_p362_m0_vs_{opponent}.json'
    sources.append(source)
    games += json.loads(source.read_text())['games']
assert len(games)==64
with (RUN/'scenarios.txt').open('x') as f:
    f.write(f'two_sided_v1 {len(games)}\n')
    for g in games:
        profile=g['opponent_profile'];fixed=sum(r[1] for r in profile['fixed_costs'])
        f.write(f"{g['seed']} {g['seat']} {g['cash']} 0 0 {len(profile['flows'])} {fixed}\n")
        f.write(' '.join(map(str,g['shops']))+'\n')
        for row in profile['flows']:f.write(' '.join(map(str,row))+'\n')
commands = [
    ['conda','run','-n','kaggriculture','g++','-std=c++20','-O3','-DNDEBUG','-march=native','-mtune=native',
     '-fno-exceptions','-fno-rtti','-fno-math-errno','-fno-semantic-interposition','-fno-plt','-flto','-I',str(ROOT),str(RUN/'search.cpp'),'-o',str(RUN/'search')],
    ['conda','run','-n','kaggriculture',str(RUN/'search'),str(RUN/'scenarios.txt'),str(RUN/'ESTIMATES.json')]]
for i,command in enumerate(commands):
    result=subprocess.run(command,capture_output=True,text=True)
    (RUN/f'prepare_{i}.log').write_text(result.stdout+result.stderr)
    result.check_returncode()
(RUN/'PREPARE_COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n')
estimates=json.loads((RUN/'ESTIMATES.json').read_text())
proposals=estimates['proposals']
original=json.loads((EXP/'runs/dated_expansion_sep08_001/proposals/dated_expansion_p362/PLAN.json').read_text())
assert proposals[0]['lives']==original['lives']
assert proposals[0]['produced']==original['produced']
assert proposals[0]['daily_hands']==original['daily_hands']
catalog=json.loads((EXP/'configs/league.json').read_text())
names=[]
for p in proposals:
    if not p['selected']:continue
    name=f"cold_renewal_p{p['id']}";names.append(name)
    folder=RUN/'proposals'/name;(folder/'source').mkdir(parents=True,exist_ok=False)
    code='inline constexpr Life lives[]={\n'+''.join('{'+','.join(map(str,l))+'},\n' for l in p['lives'])+'};\n'
    code+='inline constexpr Support support={{'+','.join(map(str,p['daily_hands']))+'},{'+','.join(map(str,p['quadrants']))+'}};\n'
    (folder/'source/plan.inc').write_text(code)
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../../joint_day_routes_sep08_001/compiler/source/agent.hpp"
namespace compositions::{name} {{
#include "plan.inc"
class Agent:public joint_day_core::AgentCore {{
public:
    Agent():AgentCore(std::vector<Life>(std::begin(lives),std::end(lives)),support,true,true,false,1,true,0,0,1) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent',
        'sources':['source/agent.cpp','../../../joint_day_routes_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
    (folder/'PLAN.json').write_text(json.dumps(p,indent=2)+'\n')
    (folder/'README.md').write_text(f"# {name}\n\nLocal dated composition, herd mode{p['herd']}, crop renewal mode{p['crop']}, at most{p['cap']} reused tiles, {p['hands']} planned hands after day13. Exact lives/support in PLAN.json. Reuses unchanged original p362 compiler; no copied external schedule. Experimental, not promoted. Parent control p0 exactly reproduces original intended plan. Operational/strength evidence and all origins in ../../LINEAGE.json and run reports.\n")
    assert name not in catalog;catalog[name]=str(folder.relative_to(ROOT))
(EXP/'configs/league.json').write_text(json.dumps(catalog,indent=2)+'\n')
lineage={'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,'parent':'joint_routes_p362_m0','reference':'empty_sale_slots_m2',
    'ideas':['Reuse temporal vacancies on existing crop tiles instead of leaving retired melon/strawberry/wheat areas empty.',
        'Search original mixed herd, replace added geese by cows or sheep, or omit them and reuse the space.',
        'Evaluate 4 crop renewal patterns, 4 tile limits and 3 workforce sizes before exact execution.'],
    'origins':['Original local p362 dated composition and compiler. No new external policy copied.',
        'Larger crop renewal motivated by fresh top-player profiles and exact cold_gap_sep08_001 production comparison.'],
    'limits':['Fixed exposed opponent flows, heuristic labor/arrival times and explicit funding deficits are approximate.',
        'The compiler may delay or miss intended births. No online shop-dependent selector or general scheduling solution is claimed.',
        'The fixed original12-worker support before day13 retains the known early funding deficit; score penalizes it and exact games enforce solvency.'],
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources+[RUN/'search.cpp',RUN/'prepare.py',
        EXP/'include/estimate.hpp',EXP/'include/biology.hpp',EXP/'include/economics.hpp',
        EXP/'runs/joint_day_routes_sep08_001/compiler/source/agent.hpp',EXP/'runs/joint_day_routes_sep08_001/compiler/source/agent.cpp']}}
(RUN/'LINEAGE.json').write_text(json.dumps(lineage,indent=2)+'\n')
print('Estimated',len(proposals),'farms x',estimates['scenarios'],'scenarios in',estimates['seconds'],'seconds; prepared',len(names),'C++ finalists.',flush=True)
for p in sorted(proposals,key=lambda p:p['score'],reverse=True)[:6]:
    print(p['id'],'herd',p['herd'],'crop',p['crop'],'new lives',p['added'],'cash',round(p['cash']),'margin',round(p['margin']),'funding',round(p['minimum']),'workgap',p['work_gap'],flush=True)
