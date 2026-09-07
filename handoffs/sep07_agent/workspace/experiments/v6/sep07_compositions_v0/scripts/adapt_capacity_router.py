"""Derive isolated C++ capacity/terminal ports from the checked shared router."""
import hashlib
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]
TARGET=EXP/"league/public_capacity_router"

GUARD=r'''
    if(step%24==23) {
        int carried=0,produced=0,consumed=0,buys=0;
        for(int u=0;u<farm.n_units;++u)for(int item=0;item<N_ITEMS;++item)carried+=std::max(0,int(obs.own.inv[u][item]));
        for(int u=0;u<std::min(tape_units,action.n_units);++u) {
            const auto& tile=farm.tiles[farm.pos_y[u]][farm.pos_x[u]];const auto cmd=action.units[u];
            if(cmd.op==OP_HARVEST)produced+=std::max(0,int(tile.yield_units));
            else if(cmd.op==OP_COLLECT_FERTILIZER && tile.fertilizer_available)++produced;
            else if(cmd.op==OP_FEED || cmd.op==OP_FERTILIZE || (cmd.op==OP_PLACE && is_animal(cmd.arg)))++consumed;
        }
        std::array<int,N_ITEMS> planned{};
        for(int m=0;m<action.n_orders;++m) {
            const auto order=action.orders[m];
            if(order.op==M_SELL)planned[order.item]+=std::max(0,order.n);
            else if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL)buys+=std::max(0,order.n);
        }
        int sells=0;
        for(int item=0;item<N_ITEMS;++item)sells+=std::min(int(obs.own.shed[item]),planned[item]);
        int needed=obs.own.shed_total+carried+produced-consumed+buys-sells-99;
        if(needed>0) {
            std::array<int,9> priority={0,1,2,3,4,5,6,7,8};
            std::sort(priority.begin(),priority.end(),[&](int a,int b) {
                bool fa=data.future[route_][step+1][a]>0,fb=data.future[route_][step+1][b]>0;
                if(fa!=fb)return fa<fb;
                if(obs.market.prices[a]!=obs.market.prices[b])return obs.market.prices[a]>obs.market.prices[b];
                return lexical[a]<lexical[b];
            });
            for(int item:priority) {
                int q=std::min(needed,std::max(0,int(obs.own.shed[item])-planned[item]));if(q<=0)continue;
                int slot=-1;
                for(int m=0;m<action.n_orders;++m)if(action.orders[m].op==M_SELL && action.orders[m].item==item) {slot=m;break;}
                if(slot>=0)action.orders[slot].n=std::max(0,action.orders[slot].n)+q;
                else if(action.n_orders<10)action.orders[action.n_orders++]={M_SELL,uint8_t(item),q};
                else continue;
                planned[item]+=q;needed-=q;if(needed<=0)break;
            }
        }
    }
'''

TERMINAL=r'''
    if(terminal_ && step==718) {
        std::array<bool,9> covered{};int kept=0;
        for(int m=0;m<action.n_orders;++m) {
            auto order=action.orders[m];
            if(order.op==M_SELL && order.item<N_PRODUCTS) {
                int item=order.item;if(covered[item] || projected[item]<=0)continue;
                order.n=projected[item];covered[item]=true;
            }
            action.orders[kept++]=order;
        }
        action.n_orders=kept;
        std::array<int,9> missing{};int n=0;
        for(int item=0;item<9;++item)if(!covered[item] && projected[item]>0)missing[n++]=item;
        std::sort(missing.begin(),missing.begin()+n,[&](int a,int b) {
            int va=projected[a]*std::max(1,obs.market.prices[a]),vb=projected[b]*std::max(1,obs.market.prices[b]);
            return va!=vb?va>vb:lexical[a]<lexical[b];
        });
        for(int i=0;i<n && action.n_orders<10;++i)action.orders[action.n_orders++]={M_SELL,uint8_t(missing[i]),projected[missing[i]]};
    }
'''


def main():
    (TARGET/"source").mkdir(parents=True,exist_ok=True)
    code=(EXP/"league/public_router/source/agent.cpp").read_text()
    code=code.replace("namespace compositions::public_router", "namespace compositions::public_capacity_router")
    code=code.replace('#include "routes.inc"','#include "../../public_router/source/routes.inc"\nconstexpr int lexical[]={7,0,6,5,3,1,4,8,2};')
    code=code.replace("void Agent::act", "void AgentCore::act")
    marker="    int available[N_ITEMS], planned[N_ITEMS]{};"
    assert code.count(marker)==1;code=code.replace(marker,GUARD+marker)
    marker="    action.finalize();\n}\n}"
    assert code.count(marker)==1;code=code.replace(marker,TERMINAL+marker)
    (TARGET/"source/agent.cpp").write_text(code)
    (TARGET/"source/agent.hpp").write_text('''#pragma once
#include "agents/common/api/agent_api.hpp"
namespace compositions::public_capacity_router {
class AgentCore {
    int route_=0;
    bool terminal_;
public:
    explicit AgentCore(bool terminal=false):terminal_(terminal) {}
    static kag::agent::AgentInfo info() {return {"public_capacity_router"};}
    void reset(const kag::agent::AgentInit&) {route_=0;}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
class Agent:public AgentCore {};
}
''')
    sources=[EXP/"research/notebooks/tetsutani/shape-the-shop-work-the-pasture-kaggriculture/extracted_MAIN_B64.py",
        EXP/"research/refresh_0504/notebooks/lynnsakurai/farming-score-a-mathematical-approach/extracted_MAIN_B64.py"]
    for terminal,name in enumerate(("public_capacity_router","public_terminal_router")):
        target=EXP/"league"/name;(target/"source").mkdir(parents=True,exist_ok=True)
        if terminal:
            (target/"source/agent.hpp").write_text('''#pragma once
#include "../../public_capacity_router/source/agent.hpp"
namespace compositions::public_terminal_router {
class Agent:public public_capacity_router::AgentCore {
public:
    Agent():AgentCore(true) {}
    static kag::agent::AgentInfo info() {return {"public_terminal_router"};}
};
}
''')
            (target/"source/agent.cpp").write_text('#include "agent.hpp"\n')
        (target/"agent.json").write_text(json.dumps({"format_version":1,"name":name,"header":"source/agent.hpp",
            "type":f"compositions::{name}::Agent","sources":["source/agent.cpp"]+(["../public_capacity_router/source/agent.cpp"] if terminal else [])},indent=2)+"\n")
        path=sources[terminal]
        (target/"IMPORT.json").write_text(json.dumps({"source":str(path.relative_to(EXP)),"source_sha256":hashlib.sha256(path.read_bytes()).hexdigest(),
            "route_origin":"Thomas Tschinkel public state router; identical decoded blob SHA aabfbe9164937e4f46b59582479cebc5a1615a5652fa804adb172742f998db11",
            "author":"Lynn Sakurai/Arlene" if terminal else "tetsutani",
            "changes":"Typed C++ port of source capacity guard"+(" and final-step liquidation" if terminal else ""),
            "reuse":"User-authorized public notebook borrowing; separate license not supplied in retrieved metadata",
            "status":"Source parity and exact strength pending; no optimization or rating claim"},indent=2)+"\n")
        (target/"README.md").write_text(f"# {name}\n\nFaithful source port of the checked Thomas router plus tetsutani's day-close\none-slot capacity reserve"+(" and Lynn Sakurai's final-step liquidation" if terminal else "")+".\nExact source hashes and origin are in IMPORT.json. The four route tables are\nbyte-identical across all sources and shared immutably; episode state is per\ninstance. No seed, hidden opponent inventory or future shop access. Source\nparity and exact comparisons are pending.\n")
    catalog_path=EXP/"configs/league.json";catalog=json.loads(catalog_path.read_text())
    for name in ("public_capacity_router","public_terminal_router"):
        catalog[name]=str((EXP/"league"/name).relative_to(EXP.parents[2]))
    catalog_path.write_text(json.dumps(catalog,indent=2)+"\n")


if __name__=="__main__":main()
