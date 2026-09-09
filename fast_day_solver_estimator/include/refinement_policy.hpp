#pragma once
#include "query_policy.hpp"
#include "context_model.hpp"

namespace labor {
enum class RefinementRule { exhaustive, first_certificate, around, utility, call_cap, time_cap };

// The first-certificate search is unchanged. Only explicit call context and
// already observed results control subsequent refinement and termination.
class RefinementPolicy {
public:
    QueryPolicy search;
    std::vector<ContextFeatures> contexts;
    RefinementRule rule;
    double threshold, refinement_cpu = 0, remaining_utility = -1;
    int refinement_calls = 0;
    bool stopped = false;

    RefinementPolicy(std::vector<ScoredProposal> values, std::vector<ContextFeatures> inputs,
                     RefinementRule strategy, double cutoff)
        : search(std::move(values), QueryOrder::around), contexts(std::move(inputs)), rule(strategy), threshold(cutoff) {
        if (contexts.size() != search.proposals.size() || !std::isfinite(threshold) || threshold < 0)
            throw std::runtime_error("invalid refinement context or cutoff");
    }

    QueryChoice next() {
        if (stopped) return {};
        if (search.best_workers < 41) {
            if (rule == RefinementRule::first_certificate ||
                (rule == RefinementRule::call_cap && refinement_calls >= threshold) ||
                (rule == RefinementRule::time_cap && refinement_cpu >= threshold)) {
                stopped = true; return {};
            }
            if (rule == RefinementRule::around || rule == RefinementRule::utility) {
                remaining_utility = -1;
                for (int index = 0; index < int(search.proposals.size()); ++index) {
                    auto& p = search.proposals[index];
                    if (p.screened) continue;
                    for (int k = p.lower; k < search.best_workers; ++k) {
                        const uint64_t bit = uint64_t{1} << k;
                        if (!(p.allowed & bit) || (p.attempted & bit)) continue;
                        if (!(p.forecasted & bit)) {
                            p.probability[k] = context_model::boost(context_query_features(contexts[index], k));
                            p.cpu[k] = 3;
                            p.forecasted |= bit; ++search.forecasts;
                        }
                        const double utility = p.probability[k] * (hire_cost(search.best_workers) - hire_cost(k)) / 3;
                        remaining_utility = std::max(remaining_utility, utility);
                    }
                }
                if (remaining_utility < threshold) { stopped = true; return {}; }
                search.order = rule == RefinementRule::utility ? QueryOrder::probability : QueryOrder::around;
            }
        }
        return search.next();
    }

    void observe(QueryChoice query, bool certified, double backend_cpu) {
        if (stopped || !std::isfinite(backend_cpu) || backend_cpu < 0)
            throw std::runtime_error("invalid refinement observation");
        const bool refining = search.best_workers < 41;
        search.observe(query, certified);
        if (refining) { ++refinement_calls; refinement_cpu += backend_cpu; }
    }
};
}
