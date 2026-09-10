#include "compile_calendar.hpp"
#include "continuation.hpp"
#include "agents/external/atakan_demand/source/base/agent.hpp"
#include <chrono>

using namespace sales_planner;
int main() {
    kag::Config config; config.seed = 202609092315;
    kag::Sim prefix(config);
    compositions::atakan_portfolio::Agent source(0);
    const auto init = kag::agent::runtime::make_agent_init(prefix, 0);
    source.reset(init);
    kag::agent::DecisionBudget budget; budget.max_expansions = 100000;
    while (prefix.st.step < 226) {
        kag::Action action, pass;
        source.act(kag::agent::runtime::make_observation(prefix, 0), budget, action);
        pass.clear(); pass.n_units = 1; pass.finalize();
        prefix.step(action, pass);
    }
    const auto start = kag::agent::runtime::make_observation(prefix, 0);
    for (int branch = 0; branch < 3; ++branch) {
        compositions::atakan_portfolio::Agent course(branch);
        const auto began = std::chrono::steady_clock::now();
        const auto compiled = compile_calendar(start, init.config, course, 202609091234, budget);
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::printf("{\"branch\":%d,\"complete\":%s,\"turns\":%d,\"projections\":%d,\"failed_work\":%d,\"seconds\":%.6f}\n",
                    branch, compiled.complete ? "true" : "false", compiled.simulated_turns,
                    compiled.projections, compiled.failed_work, seconds);
        if (!compiled.complete) return 1;
    }
}
