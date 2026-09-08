#pragma once
#include "../../../include/guarded_sequence.hpp"
#include "../../crop_value_001/proposals/crop_value_m2_t4/source/agent.hpp"

namespace catalog_rival_wool_purchase_repair_v1_compositions::productive_wheat {
class Policy {
    crop_value_m2_t4::Agent base_;
    std::vector<GuardedDay> off_,on_;
    bool entered_=false,berry_=false,preserve_;
    int selected_=-1;
    uint32_t matched_=0;
public:
    Policy(std::vector<GuardedDay>off,std::vector<GuardedDay>on,bool preserve=true):off_(std::move(off)),on_(std::move(on)),preserve_(preserve){
        if(off_.size()!=16||on_.size()!=16)std::abort();
        for(int i=0;i<16;++i)if(off_[i].plan.day!=13+i||on_[i].plan.day!=13+i)std::abort();
        const auto&a=off_[7];const auto&b=on_[7];
        if(a.tiles!=b.tiles||a.check!=b.check||a.shed!=b.shed||a.seeds!=b.seeds||a.quadrants!=b.quadrants)std::abort();
    }
    static kag::agent::AgentInfo info(){return {"productive_wheat"};}
    void reset(const kag::agent::AgentInit&i){base_.reset(i);entered_=berry_=false;selected_=-1;matched_=0;}
    uint32_t matched_days()const{return matched_;}
    bool berry_selected()const{return berry_;}
    void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&budget,kag::Action&action){
        base_.act(o,budget,action);
        if(o.hour==0){
            if(o.day==20){int demand=0;for(int i=0;i<o.n_shops;++i)
                if(kag::SHOP_MASK[o.shops[i]]&(1u<<kag::STRAWBERRY))demand+=kag::SHOP_MULT[o.shops[i]];
                berry_=preserve_&&demand>=4;}
            selected_=-1;if(o.day==13)entered_=off_[0].matches(o);
            const auto&days=berry_?on_:off_;
            if(entered_&&o.day>=13&&o.day<=28&&days[o.day-13].matches(o))selected_=o.day-13;
        }
        if(selected_<0)return;
        const auto&days=berry_?on_:off_;matched_|=uint32_t(1)<<o.day;action=days[selected_].plan.actions[o.hour];
        for(int u=action.n_units;u<o.self().n_units;++u)action.units[u]={};
        action.n_units=o.self().n_units;action.finalize();
    }
};
}
