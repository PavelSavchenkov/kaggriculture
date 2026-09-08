#include "policy.hpp"
#include "experiments/v6/sep07_compositions_v0/include/evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include <iostream>
using namespace compositions;
using namespace animal_groups_policy;
class Traced:public Policy {
    bool printed_=false;
public:
    explicit Traced(int mode):Policy(mode){}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        Policy::act(o,b,a);const auto& d=diagnostics();
        if(printed_ || o.hour!=0 || !active() || !(d.missed_days&(uint32_t{1}<<o.day)))return;
        printed_=true;std::cout<<"first_miss day="<<o.day<<" cash="<<o.self().money<<" family="<<d.family<<" choice="<<d.choice<<'\n';
        for(int leaf=0;leaf<2;++leaf){
            const auto& f=library()[d.family];const auto& g=f.choices[d.choice][leaf].days[o.day-f.first];
            std::cout<<"leaf="<<leaf<<'\n';
            for(int i=0;i<kag::N_ITEMS;++i)if(o.own.shed[i]!=g.shed[i])std::cout<<"shed "<<i<<" actual="<<o.own.shed[i]<<" expected="<<g.shed[i]<<'\n';
            for(int i=0;i<kag::N_CROPS;++i)if(o.own.seeds[i]!=g.seeds[i])std::cout<<"seed "<<i<<" actual="<<o.own.seeds[i]<<" expected="<<g.seeds[i]<<'\n';
            for(int c=0;c<100;++c){const auto actual=tile_key(o.self().tiles[c/10][c%10],o.day);
                if(!g.check[c] || actual==g.tiles[c])continue;
                std::cout<<"tile "<<c<<" actual";for(int x:actual)std::cout<<' '<<x;
                std::cout<<" expected";for(int x:g.tiles[c])std::cout<<' '<<x;std::cout<<'\n';
                for(int h=0;h<24;++h)for(int u=0;u<g.plan.actions[h].n_units;++u){
                    const auto& op=g.plan.actions[h].units[u];
                    if(op.op==kag::OP_PLANT || op.op==kag::OP_DIG)std::cout<<"job h="<<h<<" u="<<u<<" op="<<int(op.op)<<" item="<<int(op.arg)<<'\n';
                }
            }
        }
    }
};
int main(int argc,char** argv){
    if(argc!=4)return 2;Traced own(std::stoi(argv[1]));king_rc4::Agent rival;
    Options o;o.validate=true;const auto result=run_game(own,rival,std::stoull(argv[2]),std::stoi(argv[3]),o);
    for(int p=0;p<2;++p)std::cout<<"seat="<<p<<" cash="<<result.cash[p]<<" hash="<<result.hash[p]<<'\n';
}
