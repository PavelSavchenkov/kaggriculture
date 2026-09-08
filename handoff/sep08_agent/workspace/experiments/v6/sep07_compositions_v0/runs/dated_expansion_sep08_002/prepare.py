"""Preserve the prior search; test selections with no larger mean funding gap."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PARENT = EXP/'runs/dated_expansion_sep08_001'
proposals = json.loads((PARENT/'estimated/proposals.json').read_text())
limits = {p['hands']:p['min_cash'] for p in proposals if p['family']=='control'}
eligible = [p for p in proposals if p['min_cash'] >= limits[p['hands']]-1e-6]
ordered = sorted(eligible,key=lambda p:p['score'],reverse=True)
selected = {p['id'] for p in ordered[:8]}
for family in ['goose','cow','sheep','mixed','strawberry','tomato','melon','mixed_berries']:
    for hands in [8,10,12]:
        group = [p for p in ordered if p['family']==family and p['hands']==hands]
        assert group,(family,hands)
        selected.add(group[0]['id'])
selected.update([0,1,2,355,356,361,362])
catalog_path = EXP/'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
created = []
for p in proposals:
    if p['id'] not in selected:
        continue
    name = f"dated_expansion_p{p['id']}"
    names.append(name)
    if name in catalog:
        continue
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
    (folder/'README.md').write_text(f"# {name}\n\nCold dated farm: {p['family']},day{p['day']},count{p['count']},hands{p['hands']}. Chosen from prior375 estimates with no larger mean early funding gap than the matching control. This is not proof of funding feasibility: the unchanged opening itself has a small forecast deficit. Exact lives/layout/workforce in PLAN.json. Local compiler and composition; no donor schedule. Lineage in ../../LINEAGE.json. Not promoted.\n")
    catalog[name] = str(folder.relative_to(ROOT));created.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
spec = {'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,'new_packages':created,
    'baseline':'compiler_care_mixed_m1','incumbent':'observed_sale_lead_start_216',
    'parent':'runs/dated_expansion_sep08_001','proposal_data':'runs/dated_expansion_sep08_001/estimated/proposals.json',
    'eligibility':'Mean minimum cash at least the matching unchanged control across the same16 fixed-flow scenarios. The control still has a forecast deficit; no funding feasibility is certified.',
    'funding_limits':limits,'eligible_count':len(eligible),'selected_ids':sorted(selected),
    'retained_unconstrained_controls':[355,356,361,362],
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                   [PARENT/'estimated/proposals.json',PARENT/'ANALYSIS.json',RUN/'prepare.py']},
    'discovery':{'seed_start':1000,'seeds':4,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'limits':json.loads((PARENT/'LINEAGE.json').read_text())['limits']}
(RUN/'LINEAGE.json').write_text(json.dumps(spec,indent=2)+'\n')
(RUN/'run_discovery.py').write_bytes((PARENT/'run_discovery.py').read_bytes())
(RUN/'README.md').write_text('# Dated expansion with limited additional funding gaps\n\nUses prior375 exact same estimates, selects each family/workforce with mean minimum cash no lower than the unchanged opening, and retains four unconstrained leaders. Full controls use the same exposed seeds and opponents. Source packages already within this experiment are reused. This restriction is a selection experiment, not a funded lifecycle simulator or proof of feasibility.\n')
print('Eligible',len(eligible),'selected',len(names),'new packages',len(created))
