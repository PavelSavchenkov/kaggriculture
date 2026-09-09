#pragma once
#include "../compiler_placement_sep08_001/compiler/source/agent.hpp"
#include "data.hpp"
namespace compositions::cold_day_tasks {
template<int Mode> class Agent {
    compiler_placement_sep08::AgentCore base_;
    bool active_=false;
public:
    Agent():base_(lives,Support{},true,false,false,1,true,1,0) {}
    static kag::agent::AgentInfo info(){return {"cold_day_tasks"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);active_=Mode>=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a){
        base_.act(o,budget,a);
        if constexpr(Mode>=0)if(active_ && o.day==0){
            const auto& planned=days[Mode][o.hour];
            if(planned.n_units==o.self().n_units)a=planned;
            else active_=false;
        }
    }
};
}
