#include "support/evaluation.hpp"
#include "../source/agent.hpp"
#include "agents/external/teammate_shoprouter/source/agent.hpp"
#include "support/adapter.hpp"

namespace tests = bohann_catalog_tests;
using Candidate = kag::agents::bohann_opening_v1::Agent;
using Parent = kag::agents::bohann_opening_v1::detail::crop_mix_t2_wheat::Agent;
using Teammate = compositions::teammate_shoprouter::Agent;
static_assert(kag::agent::LocalAgent<Candidate>);

std::unique_ptr<tests::AgentInterface> make_investment();
std::unique_ptr<tests::AgentInterface> make_king();
std::unique_ptr<tests::AgentInterface> make_router();

class GenericAgent {
    std::unique_ptr<tests::AgentInterface> agent_;
public:
    explicit GenericAgent(const std::string& name) {
        if (name == "bohann_opening_v1") agent_ = std::make_unique<tests::Adapter<Candidate>>();
        else if (name == "crop_mix_t2_wheat") agent_ = std::make_unique<tests::Adapter<Parent>>();
        else if (name == "teammate_shoprouter") agent_ = std::make_unique<tests::Adapter<Teammate>>();
        else if (name == "investment_context_guarded_001_best") agent_ = make_investment();
        else if (name == "king_rc4") agent_ = make_king();
        else if (name == "public_router_v5") agent_ = make_router();
        else if (name == "pass") agent_ = std::make_unique<tests::Adapter<tests::Pass>>();
        else { std::fprintf(stderr, "unknown agent: %s\n", name.c_str()); std::abort(); }
    }
    void reset(const kag::agent::AgentInit& init) {
        agent_->reset(init);
    }
    void act(const kag::agent::AgentObservation& o, const kag::agent::DecisionBudget& b, kag::Action& a) {
        agent_->act(o, b, a);
    }
};

int main(int argc, char** argv) {
    const auto options = tests::options(argc, argv);
#ifdef BOHANN_TYPED_PAIR
    if (options.a != "bohann_opening_v1" || options.b != "teammate_shoprouter") return 2;
    return tests::run_batch(options, [] { return Candidate{}; }, [] { return Teammate{}; });
#else
    return tests::run_batch(options, [&] { return GenericAgent(options.a); }, [&] { return GenericAgent(options.b); });
#endif
}
