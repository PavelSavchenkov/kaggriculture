#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>

namespace compositions::destbreso_finance7 {
class AgentCore {
public:
    explicit AgentCore(int mode=3):mode_(mode) {}
    static kag::agent::AgentInfo info() {return {"destbreso_finance7"};}
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&);
    int finance_fires() const {return finance_fires_;}
    int mirror_fires() const {return mirror_fires_;}
private:
    void mirror(const kag::agent::AgentObservation&,kag::Action&);
    kag::Config config_{};
    int mode_=3,route_=0,streak_=0,mirror_route_=-1;
    std::array<int,2> scores_{};
    bool is_mirror_=false;
    int finance_fires_=0,mirror_fires_=0;
};
template<int Mode=3> class Agent:public AgentCore {
public:
    Agent():AgentCore(Mode) {}
};
}
