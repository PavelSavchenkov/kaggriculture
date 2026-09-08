#pragma once
#include "../../../../../../../common/api/agent_api.hpp"
namespace catalog_yusuke_sep08_m2_compositions::yusuke_port {
const kag::Action& planned(int route,int step);
class Policy {
    int mode_,route_=0;
public:
    explicit Policy(int mode):mode_(mode){if(mode<0 || mode>3)std::abort();}
    static kag::agent::AgentInfo info(){return {"yusuke_router"};}
    void reset(const kag::agent::AgentInit&){route_=0;}
    int selected()const{return route_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& a){
        if(o.step==0)route_=0;
        if(mode_ && o.step==144){
            bool yarn=false;for(int s=0;s<o.n_shops;++s)yarn|=o.shops[s]==kag::SHOP_YARN_STORE;
            route_=yarn?1:0;
        }
        if(o.step==648 && mode_>=2 && (mode_==2 || route_==0))
            route_=o.market.inventory[kag::EGG]<=9888?2:3;
        a.clear();a.n_units=o.self().n_units;
        if(o.step>=0 && o.step<719){
            const auto& original=planned(route_,o.step);a.n_orders=original.n_orders;
            std::copy_n(original.orders,a.n_orders,a.orders);
            std::copy_n(original.units,std::min(a.n_units,original.n_units),a.units);
        }
        a.finalize();
    }
};
}
