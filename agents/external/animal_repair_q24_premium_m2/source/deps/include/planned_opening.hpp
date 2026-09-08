#pragma once
#include <vector>
#include "../candidates/opening_router_v2/source/agent.hpp"

namespace catalog_animal_repair_q24_premium_m2_compositions {
struct DayPlan {int day;std::array<kag::Action,24> actions;};

// Complete-day replacements only in v2's delayed-hire branch. All parent
// controllers still receive observations; no offline state enters the API.
class PlannedOpeningAgent {
    opening_router_v2::Agent base_;
    std::vector<DayPlan> plans_;
    std::array<int,30> index_;
    bool selected_=false;
public:
    explicit PlannedOpeningAgent(std::vector<DayPlan> plans):plans_(std::move(plans)) {
        index_.fill(-1);
        for(int i=0;i<int(plans_.size());++i) {
            const int day=plans_[i].day;
            if(day<0 || day>=29 || index_[day]>=0)std::abort();
            index_[day]=i;
        }
    }
    static kag::agent::AgentInfo info() {return {"planned_opening"};}
    void reset(const kag::agent::AgentInit& init) {base_.reset(init);selected_=false;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if(o.step==1)selected_=o.opponent().n_units==1;
        if(!selected_ || index_[o.day]<0)return;
        action=plans_[index_[o.day]].actions[o.hour];
        for(int u=action.n_units;u<o.self().n_units;++u)action.units[u]={};
        action.n_units=o.self().n_units;action.finalize();
    }
};
}
