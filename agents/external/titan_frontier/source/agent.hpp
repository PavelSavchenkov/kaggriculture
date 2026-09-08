#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>

namespace compositions::titan_frontier {
struct Repair {int start=-100;kag::UnitAction intended{};};
class AgentCore {
    kag::agent::AgentConfig config_{};
    std::array<std::array<Repair,kag::MAX_UNITS>,3> repairs_{};
    bool overlay_;
    int route_=0,advanced_turns_=0;
public:
    explicit AgentCore(bool overlay=true):overlay_(overlay) {}
    static kag::agent::AgentInfo info(){return {"titan_frontier"};}
    void reset(const kag::agent::AgentInit& init){config_=init.config;repairs_={};route_=advanced_turns_=0;}
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
    int selected_route() const{return route_;}
    int advanced_turns() const{return advanced_turns_;}
};
class Agent:public AgentCore {
public:
    Agent():AgentCore(true) {}
};
}
