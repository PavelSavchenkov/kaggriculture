#pragma once
#include "../../../../../common/api/agent_api.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace catalog_wheat_one_fert_compositions {
// Borrowed ideas: Dusta/King latest-departure recall and Lynn full final-stock
// liquidation. Adaptation: request up to the public shed capacity for each
// product, so legal engine settlement includes this turn's deposits without
// claiming an exact warehouse projection. Existing order positions are kept.
inline void terminal_layer(const kag::agent::AgentObservation& o,kag::Action& a,int mode,int capacity=100) {
    using namespace kag;
    if((mode&2) && o.step>=708) {
        constexpr int centers[4][2]={{4,4},{5,4},{4,5},{5,5}};
        for(int u=0;u<a.n_units;++u) {
            int held=0;for(int i=0;i<N_PRODUCTS;++i)held+=o.own.inv[u][i];if(!held)continue;
            int x=o.self().pos_x[u],y=o.self().pos_y[u],best=0,distance=100;
            for(int k=0;k<4;++k) {
                int d=std::abs(x-centers[k][0])+std::abs(y-centers[k][1]);
                if(d<distance){distance=d;best=k;}
            }
            if(o.step<718-distance)continue;
            int op=distance==0?OP_DROP:x<centers[best][0]?OP_EAST:x>centers[best][0]?OP_WEST:y<centers[best][1]?OP_SOUTH:OP_NORTH;
            a.units[u]={uint8_t(op),0,1};
        }
    }
    if((mode&1) && o.step==718) {
        std::array<bool,N_PRODUCTS> covered{};
        for(int j=0;j<a.n_orders;++j)if(a.orders[j].op==M_SELL) {
            a.orders[j].n=std::max(a.orders[j].n,capacity);covered[a.orders[j].item]=true;
        }
        int visible[N_PRODUCTS]{};
        for(int i=0;i<N_PRODUCTS;++i)visible[i]=o.own.shed[i];
        for(int u=0;u<a.n_units;++u)if(is_shed_adjacent(o.self().pos_x[u],o.self().pos_y[u],BOARD)) {
            const auto& q=a.units[u];
            if(q.op==OP_DROP)for(int i=0;i<N_PRODUCTS;++i)visible[i]+=o.own.inv[u][i];
            else if(q.op==OP_PLACE && q.arg<N_PRODUCTS)visible[q.arg]+=std::min(q.n,int(o.own.inv[u][q.arg]));
        }
        std::array<int,N_PRODUCTS> order{};for(int i=0;i<N_PRODUCTS;++i)order[i]=i;
        std::stable_sort(order.begin(),order.end(),[&](int x,int y){return visible[x]*o.market.prices[x]>visible[y]*o.market.prices[y];});
        for(int i:order)if(!covered[i] && a.n_orders<10)a.orders[a.n_orders++]={M_SELL,uint8_t(i),capacity};
    }
    a.finalize();
}

template<class Base,int Mode=1> class TerminalAgent {
    Base base_;
    int capacity_=100;
public:
    TerminalAgent()=default;
    explicit TerminalAgent(Base base):base_(std::move(base)){}
    static kag::agent::AgentInfo info(){return {"terminal_overlay"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);capacity_=init.config.shed_capacity;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        base_.act(o,b,a);terminal_layer(o,a,Mode,capacity_);
    }
};
}
