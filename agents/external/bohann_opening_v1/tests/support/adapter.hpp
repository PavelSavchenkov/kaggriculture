#pragma once
#include "agents/common/api/agent_api.hpp"
#include <memory>

namespace bohann_catalog_tests {
struct AgentInterface {
    virtual ~AgentInterface() = default;
    virtual void reset(const kag::agent::AgentInit&) = 0;
    virtual void act(const kag::agent::AgentObservation&, const kag::agent::DecisionBudget&, kag::Action&) = 0;
};
template<class Policy>
class Adapter final : public AgentInterface {
    Policy policy_;
public:
    void reset(const kag::agent::AgentInit& init) override { policy_.reset(init); }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) override {
        policy_.act(o, b, a);
    }
};
}
