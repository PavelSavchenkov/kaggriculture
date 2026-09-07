#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>

namespace compositions::kaito_v58 {

struct Repair {
    int start=-100;
    kag::UnitAction intended{};
};

struct Controller {
    int last_step=-1, near_streak=0, evidence=0, n_shops=0;
    bool near=false;
    double confidence=0;
    std::array<int,9> inventory{}, own_net{};
    std::array<double,9> supply{};
    std::array<uint8_t,8> shops{};
    std::array<std::array<int,9>,720> due{};
    std::array<Repair,kag::MAX_UNITS> repairs{};
    void act(int route,const kag::agent::AgentObservation& o,
        const kag::agent::AgentConfig& config,kag::Action& action);
};

class Agent {
    kag::agent::AgentConfig config_{};
    std::array<Controller,10> controllers_{};
    int mode_=0, mirror_streak_=0;
    bool known_yarn_=false, clone_=false;
public:
    static kag::agent::AgentInfo info() { return {"kaito_v58"}; }
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action);
};

}
