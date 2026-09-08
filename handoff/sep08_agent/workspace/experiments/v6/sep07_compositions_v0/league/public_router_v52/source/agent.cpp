#include "agent.hpp"

namespace kag::agents::public_router_v52 {
namespace {
#include "data.inc"
double feature(int id,const kag::agent::AgentObservation& o) {
    if(id==2)return o.market.inventory[kag::TOMATO]-10000;
    const int product=id==24?kag::MILK:kag::WOOL;
    int demand=0;for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<product))demand+=kag::SHOP_MULT[o.shops[i]];
    return demand;
}
int choose(int block,int previous,const kag::agent::AgentObservation& o) {
    const int offset=tree_offsets[block];int node=0;
    for(;;) {
        const auto& n=nodes[offset+node];
        if(n.feature>=0)node=feature(n.feature,o)<=n.threshold?n.left:n.right;
        else if(n.feature==-2)node=previous==int(n.threshold)?n.left:n.right;
        else return n.route<0?previous:n.route;
    }
}
}
void Agent::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action) {
    action.clear();action.n_units=o.self().n_units;
    if(o.step<0 || o.step>=719){action.finalize();return;}
    if(last_step_<0 || o.step<last_step_ || o.step==0){route_=0;last_block_=-1;}
    const int block=o.step/72;
    if(block!=last_block_){route_=choose(block,route_,o);last_block_=block;}
    last_step_=o.step;
    int cursor=offsets[route_][o.step];const int units=values[cursor++];action.n_orders=values[cursor++];
    for(int u=0;u<units;++u){action.units[u]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int s=0;s<action.n_orders;++s){action.orders[s]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int u=units;u<action.n_units;++u)action.units[u]={};
    action.finalize();
}
}
