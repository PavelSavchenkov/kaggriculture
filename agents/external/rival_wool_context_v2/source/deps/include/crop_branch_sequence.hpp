#pragma once
#include "guarded_sequence.hpp"

namespace catalog_rival_wool_context_v2_compositions {
// Adapted from runs/productive_wheat_rotation_001/source/policy.hpp. Both
// complete suffixes must accept the same physical state at the branch day.
template<class Base> class CropBranchSequence {
    Base base_;
    std::vector<GuardedDay> off_,on_;
    int branch_day_,minimum_berry_demand_,selected_=-1;
    bool entered_=false,berry_=false;
    uint32_t matched_=0;
public:
    CropBranchSequence(std::vector<GuardedDay> off,std::vector<GuardedDay> on,
                       int branch_day=20,int minimum_berry_demand=4):
        off_(std::move(off)),on_(std::move(on)),branch_day_(branch_day),minimum_berry_demand_(minimum_berry_demand) {
        if(off_.empty()||off_.size()!=on_.size())std::abort();
        int branch=-1;
        for(int i=0;i<int(off_.size());++i) {
            if(off_[i].plan.day!=off_[0].plan.day+i||on_[i].plan.day!=off_[i].plan.day)std::abort();
            if(off_[i].plan.day==branch_day_)branch=i;
        }
        if(branch<0)std::abort();
        const auto& a=off_[branch];const auto& b=on_[branch];
        if(a.tiles!=b.tiles||a.check!=b.check||a.shed!=b.shed||a.seeds!=b.seeds||a.quadrants!=b.quadrants)std::abort();
    }
    static kag::agent::AgentInfo info(){return {"crop_branch_sequence"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);entered_=berry_=false;selected_=-1;matched_=0;}
    uint32_t matched_days()const{return matched_;}
    bool berry_selected()const{return berry_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if(o.hour==0) {
            if(o.day==branch_day_) {
                int demand=0;
                for(int i=0;i<o.n_shops;++i)
                    if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::STRAWBERRY))demand+=kag::SHOP_MULT[o.shops[i]];
                berry_=demand>=minimum_berry_demand_;
            }
            selected_=-1;
            if(o.day==off_[0].plan.day)entered_=off_[0].matches(o);
            const auto& days=berry_?on_:off_;
            const int index=o.day-off_[0].plan.day;
            if(entered_&&index>=0&&index<int(days.size())&&days[index].matches(o))selected_=index;
        }
        if(selected_<0)return;
        const auto& days=berry_?on_:off_;
        matched_|=uint32_t(1)<<o.day;action=days[selected_].plan.actions[o.hour];
        for(int u=action.n_units;u<o.self().n_units;++u)action.units[u]={};
        action.n_units=o.self().n_units;action.finalize();
    }
};
}
