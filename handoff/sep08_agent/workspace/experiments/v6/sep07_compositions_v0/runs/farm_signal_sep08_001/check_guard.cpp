#include "policy.hpp"
#include <iostream>

int main() {
    using namespace kag;
    int cases=0;if(!(std::cin>>cases))return 2;
    while(cases--) {
        agent::AgentObservation o{};Action a{};int route;
        std::cin>>route>>o.step>>o.farms[0].n_units>>a.n_orders;
        a.n_units=o.farms[0].n_units;
        for(int i=0;i<N_ITEMS;++i){int n;std::cin>>n;o.own.shed[i]=n;o.own.shed_total+=n;}
        for(int i=0;i<N_PRODUCTS;++i)std::cin>>o.market.prices[i];
        for(int u=0;u<a.n_units;++u) {
            int yield,fert,op,arg,n;std::cin>>yield>>fert>>op>>arg>>n;
            const int x=u%BOARD,y=u/BOARD;o.farms[0].pos_x[u]=x;o.farms[0].pos_y[u]=y;
            auto& tile=o.farms[0].tiles[y][x];tile.yield_units=yield;tile.fertilizer_available=fert;
            a.units[u]={uint8_t(op),uint8_t(arg),n};
            for(int i=0;i<N_ITEMS;++i){int count;std::cin>>count;o.own.inv[u][i]=count;}
        }
        for(int j=0;j<a.n_orders;++j){int op,item,n;std::cin>>op>>item>>n;a.orders[j]={uint8_t(op),uint8_t(item),n};}
        if(!std::cin)return 2;
        a.finalize();compositions::farm_signal::reserve(o,route,a);
        std::cout<<a.n_orders;
        for(int j=0;j<a.n_orders;++j)std::cout<<' '<<int(a.orders[j].op)<<' '<<int(a.orders[j].item)<<' '<<a.orders[j].n;
        std::cout<<'\n';
    }
}
