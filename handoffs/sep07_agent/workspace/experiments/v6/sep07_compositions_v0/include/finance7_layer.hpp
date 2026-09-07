#pragma once
#include "agents/common/api/agent_api.hpp"
#include <algorithm>
#include <cmath>

namespace compositions {
// Literal port of destbreso's _finance_action. The source uses the observed
// quote and pre-work shed, not an exact joint-market forecast. Preserve that
// approximation for the controlled source comparison.
inline bool finance7_hires(const kag::agent::AgentObservation& o,kag::Action& action) {
    if(action.n_orders==0 || action.n_orders>=10)return false;
    int hires=o.self().hires_today,cost=0;
    for(int i=0;i<action.n_orders;++i)if(action.orders[i].op==kag::M_HIRE)cost+=kag::fib(hires++);
    const double shortage=cost-o.self().money;
    if(shortage<=0)return false;
    // The Python source sorts product names before resolving equal prices.
    constexpr int alphabetical[]={kag::CARROT,kag::EGG,kag::FERTILIZER,kag::MELON,
        kag::MILK,kag::STRAWBERRY,kag::TOMATO,kag::WHEAT,kag::WOOL};
    int best=-1;
    for(int item:alphabetical)
        if(o.own.shed[item]>0 && o.market.prices[item]>0 && (best<0 || o.market.prices[item]>o.market.prices[best]))best=item;
    if(best<0)return false;
    const int units=std::min<int>(o.own.shed[best],int(std::floor(shortage/o.market.prices[best]))+1);
    if(units<1)return false;
    for(int i=action.n_orders;i>0;--i)action.orders[i]=action.orders[i-1];
    action.orders[0]={kag::M_SELL,uint8_t(best),units};++action.n_orders;return true;
}
}
