#pragma once
#include "../../league/public_router_v52/source/agent.hpp"
#include "future.hpp"
#include <algorithm>
#include <array>

namespace compositions::farm_signal {
using namespace kag;

// Faithful _room_guard translation. The public source deliberately estimates
// requested effects, including failed FEED/HARVEST; do not silently repair it.
inline bool reserve(const agent::AgentObservation& o,int route,Action& action) {
    if(o.step<0 || o.step>=719 || o.step%24!=23)return false;
    int carried=0,produced=0,consumed=0,buys=0;
    const auto& farm=o.self();
    for(int u=0;u<farm.n_units;++u) {
        for(int i=0;i<N_ITEMS;++i)carried+=std::max(0,int(o.own.inv[u][i]));
        const int x=farm.pos_x[u],y=farm.pos_y[u];
        if(x<0 || y<0 || x>=BOARD || y>=BOARD)continue;
        const auto& tile=farm.tiles[y][x];const auto a=action.units[u];
        if(a.op==OP_HARVEST)produced+=std::max(0,int(tile.yield_units));
        else if(a.op==OP_COLLECT_FERTILIZER && tile.fertilizer_available)++produced;
        else if(a.op==OP_FEED || a.op==OP_FERTILIZE || (a.op==OP_PLACE && is_animal(a.arg)))++consumed;
    }
    std::array<int,N_ITEMS> planned{};
    for(int j=0;j<action.n_orders;++j) {
        const auto order=action.orders[j];
        if(order.op==M_SELL)planned[order.item]+=std::max(0,order.n);
        else if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL)buys+=std::max(0,order.n);
    }
    int filled=0;for(int i=0;i<N_ITEMS;++i)filled+=std::min(std::max(0,int(o.own.shed[i])),planned[i]);
    int needed=o.own.shed_total+carried+produced-consumed+buys-filled-99;
    if(needed<=0)return false;
    constexpr int lexical[]={7,0,6,5,3,1,4,8,2};
    std::array<int,N_PRODUCTS> priority={0,1,2,3,4,5,6,7,8};
    const auto future=has_future[route][o.step+1];
    std::sort(priority.begin(),priority.end(),[&](int a,int b) {
        const bool fa=future&(1u<<a),fb=future&(1u<<b);
        if(fa!=fb)return fa<fb;
        if(o.market.prices[a]!=o.market.prices[b])return o.market.prices[a]>o.market.prices[b];
        return lexical[a]<lexical[b];
    });
    bool changed=false;
    for(int item:priority) {
        const int q=std::min(needed,std::max(0,int(o.own.shed[item])-planned[item]));
        if(q<=0)continue;
        int slot=-1;for(int j=0;j<action.n_orders;++j)
            if(action.orders[j].op==M_SELL && action.orders[j].item==item){slot=j;break;}
        if(slot>=0)action.orders[slot].n=std::max(0,action.orders[slot].n)+q;
        else if(action.n_orders<10)action.orders[action.n_orders++]={M_SELL,uint8_t(item),q};
        else continue;
        planned[item]+=q;needed-=q;changed=true;if(needed<=0)break;
    }
    action.finalize();return changed;
}

template<bool Enabled> class Agent:public kag::agents::public_router_v52::Agent {
    int changes_=0;
public:
    void reset(const agent::AgentInit& init){kag::agents::public_router_v52::Agent::reset(init);changes_=0;}
    int changes()const{return changes_;}
    void act(const agent::AgentObservation& o,const agent::DecisionBudget& b,Action& a) {
        kag::agents::public_router_v52::Agent::act(o,b,a);
        if constexpr(Enabled)changes_+=reserve(o,this->selected_route(),a);
    }
};
}
