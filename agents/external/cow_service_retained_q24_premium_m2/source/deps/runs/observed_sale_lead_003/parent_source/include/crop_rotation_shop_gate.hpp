#pragma once
#include "../../../../../../../../common/api/agent_api.hpp"

namespace catalog_cow_service_retained_q24_premium_m2_sale {
// One complete crop-family alternative, chosen from already observed shops.
// A failed entry contract keeps the base, including its later improvements.
template<class Base,class Changed> class CropRotationShopGate {
    Base base_;
    Changed changed_;
    int minimum_tomatoes_;
    bool selected_=false;
public:
    explicit CropRotationShopGate(int minimum_tomatoes):minimum_tomatoes_(minimum_tomatoes){}
    static kag::agent::AgentInfo info(){return {"crop_rotation_shop_gate"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);changed_.reset(init);selected_=false;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        kag::Action alternative;base_.act(o,budget,action);changed_.act(o,budget,alternative);
        if(o.day==12 && o.hour==0) {
            int demand=0;
            for(int i=0;i<o.n_shops;++i)
                demand+=bool(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::TOMATO));
            selected_=demand>=minimum_tomatoes_ && (changed_.matched_days()&(uint32_t(1)<<12));
        }
        if(selected_)action=alternative;
    }
};
}
