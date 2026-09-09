#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>

namespace compositions::v25_components {
class Agent {
    struct Pending { kag::UnitAction action; int8_t x=0,y=0; };
    struct Queue {
        std::array<Pending,720> entries{};
        int begin=0,size=0;
        void clear(){begin=size=0;}
        Pending& front(){return entries[begin];}
        void push(Pending value){entries[(begin+size)%720]=value;++size;}
        Pending pop(){auto value=front();begin=(begin+1)%720;--size;return value;}
    };
    std::array<Queue,kag::MAX_UNITS> pending_{};
    const int mask_;
    int route_=0,branch_=0,last_step_=-1,due_=-1;
    bool first_=false,production_=false,crop_=false;
    std::array<int,9> suppression_{};
    void clear_state();
public:
    explicit Agent(int mask=7):mask_(mask){}
    static kag::agent::AgentInfo info(){return {"ahmed_v25"};}
    void reset(const kag::agent::AgentInit&){clear_state();}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
};
}
