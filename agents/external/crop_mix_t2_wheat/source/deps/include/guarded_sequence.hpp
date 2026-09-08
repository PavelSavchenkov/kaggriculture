#pragma once
#include "guarded_day.hpp"

namespace catalog_crop_mix_t2_wheat_compositions {
// A changed composition must enter through its first checked day. Subsequent
// complete days are reused only when their changed physical contracts match.
template<class Base> class GuardedSequenceAgent {
    Base base_;
    std::vector<GuardedDay> days_;
    bool entered_=false;
    int selected_=-1;
    uint32_t matched_=0;
public:
    explicit GuardedSequenceAgent(std::vector<GuardedDay> days):days_(std::move(days)) {
        if(days_.empty())std::abort();
        for(size_t i=0;i<days_.size();++i)
            if(days_[i].plan.day<0 || days_[i].plan.day>=29 || (i && days_[i-1].plan.day>=days_[i].plan.day))std::abort();
    }
    static kag::agent::AgentInfo info(){return {"guarded_sequence"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);entered_=false;selected_=-1;matched_=0;}
    uint32_t matched_days() const{return matched_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if(o.hour==0) {
            selected_=-1;
            if(o.day==days_[0].plan.day)entered_=days_[0].matches(o);
            if(entered_)for(int i=0;i<int(days_.size());++i)
                if(days_[i].plan.day==o.day && days_[i].matches(o)){selected_=i;break;}
        }
        if(selected_<0)return;
        matched_|=uint32_t(1)<<o.day;action=days_[selected_].plan.actions[o.hour];
        for(int u=action.n_units;u<o.self().n_units;++u)action.units[u]={};
        action.n_units=o.self().n_units;action.finalize();
    }
};
}
