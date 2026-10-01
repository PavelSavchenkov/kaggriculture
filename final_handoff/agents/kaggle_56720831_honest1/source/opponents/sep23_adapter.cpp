#include "sep23_adapter.hpp"
#include "source/agent.hpp"

namespace sep23 {
struct Opponent::Impl { kag::agents::agent_sep23::Agent agent; };
Opponent::Opponent() : impl_(std::make_unique<Impl>()) {}
Opponent::~Opponent() = default;
void Opponent::reset(const kag::agent::AgentInit& init) { impl_->agent.reset(init); }
void Opponent::act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) {
    impl_->agent.act(o, b, a);
}
}
