#pragma once
#include "supply_bounds.hpp"
#include "search_model.hpp"
#include "original_labor.hpp"
#include <limits>
#include <tuple>

namespace labor {
enum class QueryOrder { sequential, pruned, around, global_descending, probability, hybrid };
enum class CostModel { direct, original_geometry, flat_operations };

struct ScoredProposal {
    Features features{};
    int lower = 1;
    bool screened = false;
    double workers = 1;
    // Tie ranks are supplied by the outer search. The frozen experiment uses
    // ranks of SHA256(physical ID), with separate ranks for query IDs.
    unsigned tie = 0;
    std::array<unsigned, 41> query_tie{};
    uint64_t allowed = 0, attempted = 0;
    std::array<double, 41> probability{}, cpu{};
    uint64_t forecasted = 0;
};

inline double cost_equivalent_workers(double cost) {
    cost = std::max(0.0, cost);
    for (int workers = 1; workers < 40; ++workers) {
        const double before = hire_cost(workers), after = hire_cost(workers + 1);
        if (cost <= after) return workers + (cost - before) / (after - before);
    }
    return 40;
}

inline ScoredProposal score_proposal(const day_solver::DayProblem& p, int active_hours, uint64_t allowed, unsigned tie,
                                     CostModel model = CostModel::direct) {
    if (active_hours != 24) throw std::runtime_error("this cost/query model supports only 24 real phases");
    if ((allowed & 1) || (allowed >> 41)) throw std::runtime_error("query workforce outside 1..40");
    ScoredProposal result; result.allowed = allowed; result.tie = tie;
    const auto menu = earliest_menu(p);
    const auto supply = supply_bound(p, menu);
    if (supply.missing || supply.workers > 40) { result.screened = true; result.lower = 41; return result; }
    result.features = extract(p);
    result.lower = std::max(supply.workers, workforce_lower_bound(p, result.features, menu));
    result.screened = result.lower > 40 || result.features[deadline_missing_quantity] > 0;
    if (!result.screened) {
        if (model == CostModel::original_geometry) result.workers = original::estimate(original::aggregate(p)).workers;
        else if (model == CostModel::flat_operations) result.workers = 10 * result.features[tasks];
        else result.workers = std::max(double(result.lower), cost_equivalent_workers(search_model::cost(result.features)));
    }
    return result;
}

struct QueryChoice {
    int proposal = -1, workers = 0;
    friend bool operator==(const QueryChoice&, const QueryChoice&) = default;
};

// Calls no solver. The caller returns a strictly verified success or a bounded
// failure after running the chosen query. A failure never proves infeasibility.
class QueryPolicy {
public:
    std::vector<ScoredProposal> proposals;
    QueryOrder order;
    bool use_cpu = false, use_logistic = false, deferred = false;
    int best_workers = 41, forecasts = 0;

    QueryPolicy(std::vector<ScoredProposal> values, QueryOrder strategy, bool cpu = false, bool logistic = false)
        : proposals(std::move(values)), order(strategy), use_cpu(cpu), use_logistic(logistic) {}

    QueryChoice next() {
        QueryChoice chosen;
        const bool predictive = order == QueryOrder::probability || (order == QueryOrder::hybrid && deferred);
        double best_utility = -1;
        std::tuple<double, unsigned, double, int> best_rank;
        unsigned best_tie = 0;
        for (int index = 0; index < int(proposals.size()); ++index) {
            auto& p = proposals[index];
            if (p.screened) continue;
            for (int workers = 1; workers <= 40; ++workers) {
                const uint64_t bit = uint64_t{1} << workers;
                if (!(p.allowed & bit) || (p.attempted & bit)) continue;
                if (order != QueryOrder::sequential && (workers < p.lower || workers >= best_workers)) continue;
                if (predictive) {
                    if (!(p.forecasted & bit)) {
                        const auto x = search_model::query_features(p.features, workers, p.lower);
                        p.probability[workers] = use_logistic ? search_model::logistic(x) : search_model::boost(x);
                        p.cpu[workers] = use_cpu ? std::max(.0001, search_model::cpu(x)) : 3;
                        p.forecasted |= bit; ++forecasts;
                    }
                    const double gain = best_workers == 41 ? 1 : hire_cost(best_workers) - hire_cost(workers);
                    const double denominator = use_cpu ? std::max(.05, p.cpu[workers]) : 3;
                    const double utility = p.probability[workers] * gain / denominator;
                    if (chosen.proposal < 0 || utility > best_utility || (utility == best_utility && p.query_tie[workers] < best_tie)) {
                        chosen = {index, workers}; best_utility = utility; best_tie = p.query_tie[workers];
                    }
                } else {
                    std::tuple<double, unsigned, double, int> rank;
                    if (order == QueryOrder::global_descending)
                        rank = {-double(workers), 0, p.workers, int(p.tie)};
                    else if (order == QueryOrder::around || order == QueryOrder::hybrid)
                        rank = {p.workers, p.tie, std::abs(workers - p.workers), -workers};
                    else rank = {p.workers, p.tie, -double(workers), 0};
                    if (chosen.proposal < 0 || rank < best_rank) { chosen = {index, workers}; best_rank = rank; }
                }
            }
        }
        return chosen;
    }

    void observe(QueryChoice query, bool certified) {
        if (query.proposal < 0 || query.proposal >= int(proposals.size()) || query.workers < 1 || query.workers > 40)
            throw std::runtime_error("invalid observed query");
        auto& p = proposals[query.proposal]; const uint64_t bit = uint64_t{1} << query.workers;
        if (!(p.allowed & bit) || (p.attempted & bit)) throw std::runtime_error("query not allowed or already observed");
        p.attempted |= bit;
        if (certified) best_workers = std::min(best_workers, query.workers);
        else if (order == QueryOrder::hybrid) deferred = true;
    }
};
}
