#include "policy.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <iostream>
int main(){
    using namespace kag;using namespace compositions::yusuke_port;
    for(int r=0;r<4;++r)for(int t=0;t<719;++t){
        const auto& a=planned(r,t);std::cout<<a.n_units<<' '<<a.n_orders;
        for(int u=0;u<a.n_units;++u)std::cout<<' '<<+a.units[u].op<<' '<<+a.units[u].arg<<' '<<a.units[u].n;
        for(int s=0;s<a.n_orders;++s)std::cout<<' '<<+a.orders[s].op<<' '<<+a.orders[s].item<<' '<<a.orders[s].n;
        std::cout<<'\n';
    }
    Sim sim;const auto init=agent::runtime::make_agent_init(sim,0);agent::DecisionBudget budget;
    for(int mode=0;mode<4;++mode){Policy p(mode);
        for(int yarn=0;yarn<2;++yarn)for(int stock:{9887,9888,9889}){
            p.reset(init);auto o=agent::runtime::make_observation(sim,0);o.step=144;o.day=6;
            o.n_shops=2;o.shops[0]=SHOP_BAKERY;o.shops[1]=yarn?SHOP_YARN_STORE:SHOP_BAKERY;
            Action a;p.act(o,budget,a);const int early=p.selected();
            o.step=648;o.day=27;o.market.inventory[EGG]=stock;p.act(o,budget,a);
            std::cout<<mode<<' '<<yarn<<' '<<stock<<' '<<early<<' '<<p.selected()<<'\n';
            p.reset(init);if(p.selected()!=0)std::abort();
        }
    }
}
