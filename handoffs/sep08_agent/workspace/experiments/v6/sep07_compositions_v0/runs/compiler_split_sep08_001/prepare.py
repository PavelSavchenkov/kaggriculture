"""Separate worker planning from market projection with exact old controls."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
PARENT=EXP/'runs/compiler_care_sep08_001'


def replace(text,old,new):
    assert text.count(old)==1,old
    return text.replace(old,new)


core=RUN/'compiler';(core/'source').mkdir(parents=True,exist_ok=False)
h=(PARENT/'compiler/source/agent.hpp').read_text().replace('compiler_care_sep08','compiler_split_sep08')
h=replace(h,'class AgentCore {\npublic:', '''class AgentCore {
public:
    struct MarketContext {
        std::array<int,100> soon;
        int feed_needed=0,fertilizer_needed=0,job_count=0;
    };''')
h=replace(h,'    void act(const kag::agent::AgentObservation& observation,const kag::agent::DecisionBudget&,kag::Action& action);', '''    void act(const kag::agent::AgentObservation& observation,const kag::agent::DecisionBudget&,kag::Action& action);
    void plan_units(const kag::agent::AgentObservation&,kag::Action&,MarketContext&);
    void plan_market(const kag::agent::AgentObservation&,kag::Action&,const MarketContext&);''')
(core/'source/agent.hpp').write_text(h)
s=(PARENT/'compiler/source/agent.cpp').read_text().replace('compiler_care_sep08','compiler_split_sep08')
s=replace(s,'void AgentCore::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action) {', '''void AgentCore::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action) {
    MarketContext context;
    plan_units(o,action,context);
    plan_market(o,action,context);
}

void AgentCore::plan_units(const kag::agent::AgentObservation& o,kag::Action& action,MarketContext& context) {''')
s=replace(s,'    std::array<int,100> desired,soon,existing;desired.fill(-1);soon.fill(-1);existing.fill(-1);',
    '    std::array<int,100> desired,existing;auto& soon=context.soon;desired.fill(-1);soon.fill(-1);existing.fill(-1);')
s=replace(s,'    Projection projected(o,action);', '''    context.feed_needed=feed_needed;context.fertilizer_needed=fertilizer_needed;context.job_count=jobs.size;
}

// Project the actual supplied unit actions before choosing any market order.
// Alternative physical planners must preserve unit count and legal actions.
void AgentCore::plan_market(const kag::agent::AgentObservation& o,kag::Action& action,const MarketContext& context) {
    using namespace kag;
    const auto& farm=o.self();const auto& soon=context.soon;
    const int feed_needed=context.feed_needed,fertilizer_needed=context.fertilizer_needed;
    Projection projected(o,action);''')
s=s.replace('std::ceil(jobs.size*2.3/22)','std::ceil(context.job_count*2.3/22)')
s=s.replace('std::max(2,jobs.size/3)','std::max(2,context.job_count/3)')
(core/'source/agent.cpp').write_text(s)
(core/'agent.json').write_text(json.dumps({'format_version':1,'name':'compiler_split_sep08_core','header':'source/agent.hpp',
    'type':'compositions::compiler_split_sep08::Agent<>','sources':['source/agent.cpp']},indent=2)+'\n')
(core/'README.md').write_text('# Physical and market planning boundary\n\nplan_units returns the old physical actions and the market context. plan_market projects whichever physical actions it receives, then emits the old market policy. Default act calls them consecutively. This refactor changes no strategy or parameters; exact controls are required before plugging in a different route planner. Context and all episode state are per instance/call.\n')
catalog_path=EXP/'configs/league.json';catalog=json.loads(catalog_path.read_text());names=[]
for case in ['mixed','goose','p355','p362','p4','p55']:
    name=f'compiler_split_{case}';folder=RUN/'proposals'/name;(folder/'source').mkdir(parents=True,exist_ok=False)
    includes='#include "../../../../compiler_labor_sep08_001/cold_farm.hpp"\n'
    if case in ['mixed','goose']:
        counts='2,2,0,7,12,8' if case=='mixed' else '0,0,6,12,0,0';labor=1 if case=='mixed' else 3
        args=f'compiler_labor_data::cold_farm({counts}),Support{{}},false,false,false,1,true,{labor},1,1'
    elif case in ['p355','p362']:
        parent=f'dated_expansion_{case}'
        includes+=f'#include "../../../../dated_expansion_sep08_001/proposals/{parent}/source/agent.hpp"\n'
        args=f'std::vector<Life>(std::begin({parent}::lives),std::end({parent}::lives)),{parent}::support,true,true,false,1,true,0,0,1'
    else:
        args=f'{case[1:]},true,true,true,1,true,0,0,1'
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../compiler/source/agent.hpp"
{includes}namespace compositions::{name} {{
class Agent:public compiler_split_sep08::AgentCore {{
public:
    Agent():AgentCore({args}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../compiler/source/agent.cpp']},indent=2)+'\n')
    (folder/'README.md').write_text(f'# {name}\n\nUnchanged {case} farm and policy through the split physical/market API. Large cold plans are local; p4/p55 retain prior exact replay source lineage and recorded support/service. All behavior must match prior full records. No strength claim. See ../../LINEAGE.json.\n')
    assert name not in catalog;catalog[name]=str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,
    'discovery':{'seed_start':1000,'seeds':8,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'parent':'runs/compiler_care_sep08_001','source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
        [PARENT/'compiler/source/agent.hpp',PARENT/'compiler/source/agent.cpp',RUN/'prepare.py']},
    'purpose':'Create a clean physical/market boundary before adding reusable route and input assignments. Require complete old game records, not only final rewards, to match.'},indent=2)+'\n')
(RUN/'run_discovery.py').write_bytes((PARENT/'run_discovery.py').read_bytes())
print('Prepared six split-planner control policies.')
