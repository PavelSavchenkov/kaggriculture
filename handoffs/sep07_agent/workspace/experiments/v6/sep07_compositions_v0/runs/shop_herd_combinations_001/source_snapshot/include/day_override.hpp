#pragma once
#include <array>
#include "../league/top_replay_library/source/agent.hpp"

namespace compositions {
class DayOverrideAgent {
public:
    DayOverrideAgent(int program,int day,const std::array<kag::Action,24>& actions)
        :base_(program),day_(day),actions_(actions) {}
    static kag::agent::AgentInfo info() {return {"day_override"};}
    void reset(const kag::agent::AgentInit& init) {base_.reset(init);}
    void act(const kag::agent::AgentObservation& observation,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        if(observation.day!=day_) {base_.act(observation,budget,action);return;}
        action=actions_[observation.hour];
        for(int u=action.n_units;u<observation.self().n_units;++u)action.units[u]={};
        action.n_units=observation.self().n_units;action.finalize();
    }
private:
    top_replay_library::Agent base_;
    int day_;
    std::array<kag::Action,24> actions_;
};
}
