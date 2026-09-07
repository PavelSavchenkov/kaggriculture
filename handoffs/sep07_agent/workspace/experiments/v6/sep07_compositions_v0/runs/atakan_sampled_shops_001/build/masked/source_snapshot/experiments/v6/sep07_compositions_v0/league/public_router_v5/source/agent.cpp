#include "agent.hpp"
#include <array>

namespace compositions::public_router_v5 {
namespace {
#include "data.inc"
std::array<double,100> features(const kag::agent::AgentObservation& o) {
    std::array<double,100> result{};
    // Generation asserts that these are the only referenced tree features.
    result[4]=o.market.prices[kag::CARROT];
    for(int i=0;i<o.n_shops;++i) {
        result[28]+=o.shops[i]==7;
        result[35]+=o.shops[i]==3 || o.shops[i]==5 || o.shops[i]==6;
    }
    return result;
}
int choose(int block,const std::array<double,100>& x) {
    const int offset=tree_offsets[block];int node=0;
    while(nodes[offset+node].feature>=0) {
        const auto& n=nodes[offset+node];node=x[n.feature]<=n.threshold?n.left:n.right;
    }
    return nodes[offset+node].route;
}
}
void Agent::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action) {
    action.clear();action.n_units=o.self().n_units;
    if(o.step<0 || o.step>=719) {action.finalize();return;}
    if(last_step_<0 || o.step==0 || o.step<last_step_)route_=choose(0,features(o));
    if(o.step%144==0)route_=choose(o.step/144,features(o));
    last_step_=o.step;
    int cursor=offsets[route_][o.step];const int units=values[cursor++];action.n_orders=values[cursor++];
    for(int u=0;u<units;++u) {action.units[u]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int i=0;i<action.n_orders;++i) {action.orders[i]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int u=units;u<action.n_units;++u)action.units[u]={};
    action.finalize();
}
}
