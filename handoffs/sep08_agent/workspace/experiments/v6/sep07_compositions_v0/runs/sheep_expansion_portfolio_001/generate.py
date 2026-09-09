"""Format immutable replay data and C++ packages, never run gameplay in Python."""
import csv
import hashlib
import json
import sys
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
sys.path.insert(0,str(EXP/'scripts'))
from generate_public_routes import ITEMS,triple
from verify_public_router import pack

NAMES=['sheep_fixed_small','sheep_fixed_expansion','sheep_yarn2','sheep_yarn3','sheep_value_own_mean','sheep_value_margin_mean','sheep_value_own_s64','sheep_value_margin_s64']


def main():
    (RUN/'source').mkdir(exist_ok=True);(RUN/'tests').mkdir(exist_ok=True)
    reference=EXP/'research/refresh_1342/sheep_expansion';values=[];offsets=[];models=[];donors=[]
    for branch,label in enumerate(['small','expansion']):
        folder=reference/label;source=json.loads((folder/'IMPORT.json').read_text());episode=source['source']['episode'];seat=source['source']['seat']
        replay=json.loads((EXP/f'replays/episode-{episode}-replay.json').read_text());actions=json.loads((folder/'raw_actions_719.json').read_text());row=[]
        with (RUN/f'tests/source_{branch}.txt').open('w')as fixture:
            for step,action in enumerate(actions):
                units=[triple(x)for x in [action['farmer'],*action['hands']]];orders=[triple(x,True)for x in action['market']];row.append(len(values));values += [len(units),len(orders),*(v for t in units+orders for v in t)]
                fixture.write(pack(dict(replay['steps'][step][seat]['observation'],step=step),action,step==0,full_tiles=True))
        offsets.append(row);sales=[[0]*9 for _ in range(30)];buys=[[0]*9 for _ in range(30)];fixed=[0.]*30
        for t in csv.DictReader((folder/'transactions.csv').open()):
            d=int(t['day'])
            if d<12:continue
            if t['operation']=='SELL':sales[d][ITEMS.index(t['item'])]+=int(t['actual'])
            elif t['operation']=='BUY_PRODUCT':buys[d][ITEMS.index(t['item'])]+=int(t['actual'])
            else:fixed[d]+=float(t['value'])
        models.append({'branch':branch,'sales':sales,'buys':buys,'fixed_costs':fixed});donors.append(source)
    code='// Public recorded courses and intended own-flow templates; lineage in LINEAGE.json.\n'
    code+='inline constexpr int offsets[2][719]='+json.dumps(offsets,separators=(',',':')).replace('[','{').replace(']','}')+';\n'
    code+='inline constexpr int values[]={\n'+',\n'.join(','.join(map(str,values[i:i+100]))for i in range(0,len(values),100))+'\n};\n'
    for key,name in [('sales','planned_sales'),('buys','planned_buys')]:code+=f'inline constexpr int {name}[2][30][9]='+json.dumps([m[key]for m in models],separators=(',',':')).replace('[','{').replace(']','}')+';\n'
    code+='inline constexpr double planned_fixed[2][30]='+json.dumps([m['fixed_costs']for m in models],separators=(',',':')).replace('[','{').replace(']','}')+';\n'
    (RUN/'source/data.inc').write_text(code);(RUN/'flow_models.json').write_text(json.dumps(models,indent=2)+'\n')
    previous=EXP/'runs/atakan_sampled_shops_001/source/agent.cpp';body=previous.read_text().replace('atakan_integrated','sheep_portfolio').replace('std::array<Estimate,3>','std::array<Estimate,2>').replace('branch<3','branch<2').replace('known[p]=1;','known[p]=p!=kag::FERTILIZER;').replace('double demand=1;','double demand=p!=kag::FERTILIZER;')
    old='if(o.step==226){estimates_=estimate_all(o,count_);branch_=0;double best=-1e100;for(int branch=0;branch<2;++branch){const auto&v=estimates_[branch];const double score=v.own-(margin_?v.rival:0);if(score>best){best=score;branch_=branch;}}}'
    new='''if(o.step==288){
        if(mode_==2||mode_==3){int yarn=0;for(int i=0;i<o.n_shops;++i)yarn+=o.shops[i]==kag::SHOP_YARN_STORE;branch_=yarn>=mode_;}
        else if(mode_>=4){estimates_=estimate_all(o,mode_>=6?64:1);const bool margin=mode_==5||mode_==7;const double a=estimates_[0].own-(margin?estimates_[0].rival:0),b=estimates_[1].own-(margin?estimates_[1].rival:0);branch_=b>a;}
    }'''
    assert old in body;body=body.replace(old,new);(RUN/'source/agent.cpp').write_text(body)
    (RUN/'source/agent.hpp').write_text('''#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>
namespace compositions::sheep_portfolio {
struct Estimate {double own=0,rival=0,min_cash=0;};
std::array<Estimate,2> estimate_all(const kag::agent::AgentObservation&,int count);
class Agent {
    int mode_=0,branch_=0;std::array<Estimate,2> estimates_{};
public:
    explicit Agent(int mode=0):mode_(mode),branch_(mode<2?mode:0){}
    static kag::agent::AgentInfo info(){return {"sheep_portfolio"};}
    void reset(const kag::agent::AgentInit&){branch_=mode_<2?mode_:0;estimates_={};}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
    int branch()const{return branch_;}const auto&estimates()const{return estimates_;}
};}
''')
    source_audit=EXP/'runs/atakan_portfolio_001/source/audit.hpp';(RUN/'source/audit.hpp').write_text(source_audit.read_text().replace('atakan_portfolio','sheep_portfolio'))
    lineage={'donors':donors,'shared_normalized_prefix':288,'selector_origin':'Locally inferred observed Yarn count thresholds or local full-own-flow valuation; no private donor formula known.','model_origin':str(previous.relative_to(EXP)),'model_origin_sha256':hashlib.sha256(previous.read_bytes()).hexdigest(),'model_changes':['Two full courses, decision at day12 hour0, complete remaining own trades and fixed costs','Correct town-center demand: no fertilizer consumption','Mean or64 observation-only stratified future shop sequences; no environment seed'], 'limits':['Donor fixed flows are forecasts, not guaranteed live realization','Rival current visible herd only, no private inventory or future expansion/crops','Daily midpoint prices/netted trades approximate intraday effects','No hard cash/route feasibility certificate','Own donor states differ by cash and10shedwheat despite identical actions before288'],'source_files_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()for p in [previous,source_audit,reference/'COMPARISON.json']}}
    (RUN/'LINEAGE.json').write_text(json.dumps(lineage,indent=2,ensure_ascii=False)+'\n')
    for mode,name in enumerate(NAMES):
        p=RUN/'proposals'/name;(p/'source').mkdir(parents=True,exist_ok=True)
        (p/'source/agent.hpp').write_text('#pragma once\n#include "../../../source/agent.hpp"\n'+f'namespace compositions::{name}{{class Agent:public sheep_portfolio::Agent{{public:Agent():sheep_portfolio::Agent({mode}){{}}static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
        (p/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (p/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../source/agent.cpp']},indent=2)+'\n')
        (p/'IMPORT.json').write_text(json.dumps({'lineage':'../../LINEAGE.json','mode':mode,'name':name,'status':'Unvalidated experiment'},indent=2)+'\n');(p/'README.md').write_text(f'# {name}\n\nTwo complete public courses; mode{mode}. See ../../LINEAGE.json. Unvalidated experimental policy.\n')
    print('Generated two courses and eight C++ variants.')


if __name__=='__main__':main()
