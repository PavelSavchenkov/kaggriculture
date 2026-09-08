#pragma once
#include "../../../../../common/api/agent_api.hpp"

namespace catalog_crop_rotation_t2_berry_compositions {
// Select one complete checked crop continuation at its entry observation.
// Both independent controllers consume the same real observations. The cheap
// rule is a heuristic, not a forecast of future shops or guaranteed profit.
template<class Base,class Changed> class CropValueGate {
    Base base_;
    Changed changed_;
    int mode_,threshold_;
    bool selected_=false;
public:
    CropValueGate(int mode,int threshold):mode_(mode),threshold_(threshold){}
    static kag::agent::AgentInfo info(){return {"crop_value_gate"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);changed_.reset(init);selected_=false;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        if(o.day==20 && o.hour==0) {
            const int berry=kag::market_price(kag::STRAWBERRY,o.market.inventory[kag::STRAWBERRY]);
            const int fertilizer=kag::market_price(kag::FERTILIZER,o.market.inventory[kag::FERTILIZER]);
            int demand=0;
            for(int i=0;i<o.n_shops;++i)if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::STRAWBERRY))demand+=kag::SHOP_MULT[o.shops[i]];
            selected_=mode_==0 || (mode_==1 && 2*berry-fertilizer>threshold_) ||
                (mode_==2 && demand>=threshold_);
        }
        kag::Action alternative;base_.act(o,budget,action);changed_.act(o,budget,alternative);
        if(selected_)action=alternative;
    }
};
}
