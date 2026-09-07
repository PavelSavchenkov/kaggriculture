#pragma once

// PyVRP 0.14.0's Python solve/ILS/penalty orchestration, ported to C++.
// See PYVRP_LICENSE.md. The pinned native search implementation is unchanged.
#include "routing_data.hpp"
#include "Solution.h"
#include "search/LocalSearch.h"
#include "search/neighbourhood.h"
#include "search/Relocate.h"
#include "search/Swap.h"
#include "search/SwapTails.h"
#include "search/RelocateAlternative.h"
#include "search/RelocatePickup.h"
#include "search/RelocateDelivery.h"
#include "search/RelocateWithDepot.h"
#include "search/RemoveAdjacentDepot.h"
#include "search/RemoveOptionalClient.h"
#include "search/InsertOptionalClient.h"
#include "search/ReplaceOptionalClient.h"
#include "search/RemoveOptionalShipment.h"
#include "search/InsertOptionalShipment.h"
#include "search/ReplaceOptionalShipment.h"
#include "search/ReplaceGroup.h"
#include "search/RelocateShipment.h"
#include <functional>
#include <memory>

namespace day_constructor {
namespace vrp = pyvrp;
namespace ls = pyvrp::search;
using SolutionPtr = std::shared_ptr<const vrp::Solution>;

struct SearchOptions {
    int iterations = 5000, restart_after = 150000, history_length = 300;
    bool exhaustive_on_best = true;
};

class Penalties {
    int registered_ = 0;
    std::vector<int> feasible_;
public:
    std::vector<double> values;
    explicit Penalties(size_t loads) : feasible_(loads + 2), values(loads + 2, 0.1 + (100000.0 - 0.1) / 2) {}
    vrp::CostEvaluator evaluator(bool maximum = false) const {
        auto copy = values;
        if (maximum) std::fill(copy.begin(), copy.end(), 100000.0);
        return {{copy.begin(), copy.end() - 2}, copy[copy.size() - 2], copy.back()};
    }
    void register_solution(const vrp::Solution& solution) {
        std::vector<Count> violations;
        for (auto load : solution.excessLoad()) violations.push_back(Count(load));
        violations.push_back(Count(solution.timeWarp()));
        violations.push_back(Count(solution.excessDistance()));
        require(violations.size() == values.size(), "penalty dimension mismatch");
        for (size_t i = 0; i < values.size(); ++i) feasible_[i] += violations[i] == 0;
        if (++registered_ != 500) return;
        for (size_t i = 0; i < values.size(); ++i) {
            const double difference = 0.65 - double(feasible_[i]) / 500;
            if (std::abs(difference) >= 0.05)
                values[i] = std::clamp(values[i] * (difference > 0 ? 1.5 : 0.9), 0.1, 100000.0);
            feasible_[i] = 0;
        }
        registered_ = 0;
        // Python also tracks average violations for warning messages only.
    }
};

using SearchObserver = std::function<void(int, const vrp::Solution&, const vrp::Solution&, const vrp::Solution&,
                                          const vrp::CostEvaluator&, const Penalties&, const vrp::RandomNumberGenerator&)>;

class Search {
    const vrp::ProblemData& data_;
    vrp::RandomNumberGenerator rng_;
    ls::PerturbationManager perturbation_;
    std::vector<std::unique_ptr<ls::UnaryOperator>> unary_;
    std::vector<std::unique_ptr<ls::BinaryOperator>> binary_;
    ls::LocalSearch search_;
    template <class Operator> void add() {
        if (!Operator::supports(data_)) return;
        auto op = std::make_unique<Operator>(data_);
        search_.addOperator(*op);
        if constexpr (std::is_base_of_v<ls::UnaryOperator, Operator>) unary_.push_back(std::move(op));
        else binary_.push_back(std::move(op));
    }
    SolutionPtr improve(const vrp::Solution& solution, const vrp::CostEvaluator& evaluator, bool exhaustive) {
        search_.shuffle(rng_);
        ++search_calls;
        return std::make_shared<vrp::Solution>(search_(solution, evaluator, exhaustive));
    }
public:
    int search_calls = 0, restarts = 0, best_updates = 0, accepted = 0, history_updates = 0, iterations_run = 0;
    explicit Search(const vrp::ProblemData& data, uint32_t seed)
        : data_(data), rng_(seed), search_(data, ls::computeNeighbours(data, {}), perturbation_) {
        // Same order as pyvrp.search.OPERATORS, including supports filtering.
        add<ls::Relocate<1>>(); add<ls::Relocate<2>>();
        add<ls::Swap<1, 1>>(); add<ls::Swap<2, 1>>(); add<ls::Swap<2, 2>>();
        add<ls::SwapTails>(); add<ls::RelocateAlternative>();
        add<ls::RelocatePickup>(); add<ls::RelocateDelivery>();
        add<ls::RelocateWithDepot>(); add<ls::RemoveAdjacentDepot>();
        add<ls::RemoveOptionalClient>(); add<ls::InsertOptionalClient>(); add<ls::ReplaceOptionalClient>();
        add<ls::RemoveOptionalShipment>(); add<ls::InsertOptionalShipment>(); add<ls::ReplaceOptionalShipment>();
        add<ls::ReplaceGroup>(); add<ls::RelocateShipment>();
    }
    const auto& neighbours() const { return search_.neighbours(); }
    const auto& rng_state() const { return rng_.state(); }

    SolutionPtr run(SearchOptions options = {}, const SearchObserver& observer = {},
                    const std::function<bool()>& stop = {}) {
        require(options.iterations >= 0 && options.restart_after >= 0 && options.history_length > 0, "invalid search settings");
        require(search_calls == 0, "search instance must run once");
        Penalties penalties(data_.numLoadDimensions());
        const vrp::Solution random(data_, rng_);
        const auto initial = improve(random, penalties.evaluator(true), true);
        auto current = initial, best = initial;
        std::vector<SolutionPtr> history(options.history_length);
        int index = 0, without_improvement = 0;
        if (observer) observer(0, *initial, *initial, *initial, penalties.evaluator(), penalties, rng_);
        for (int iteration = 1; iteration <= options.iterations; ++iteration) {
            if (stop && stop()) break;
            iterations_run = iteration;
            if (without_improvement == options.restart_after) {
                std::fill(history.begin(), history.end(), nullptr);
                index = 0;
                current = best;
                without_improvement = 0;
                ++restarts;
            }
            // This evaluator remains unchanged until the next iteration,
            // even when register_solution updates penalties below.
            const auto evaluator = penalties.evaluator();
            auto candidate = improve(*current, evaluator, false);
            penalties.register_solution(*candidate);
            ++without_improvement;
            if (evaluator.cost(*candidate) < evaluator.cost(*best)) {
                best = candidate;
                without_improvement = 0;
                ++best_updates;
                if (options.exhaustive_on_best) {
                    candidate = improve(*candidate, evaluator, true);
                    if (candidate->isFeasible()) best = candidate;
                }
            }
            const auto candidate_cost = evaluator.penalisedCost(*candidate);
            auto current_cost = evaluator.penalisedCost(*current);
            const auto& late = history[index];
            const auto late_cost = evaluator.penalisedCost(late ? *late : *initial);
            if (candidate_cost < late_cost || candidate_cost < current_cost) {
                current = candidate;
                current_cost = candidate_cost;
                ++accepted;
            }
            if (current_cost < late_cost || !late) { history[index] = current; ++history_updates; }
            index = (index + 1) % options.history_length;
            if (observer) observer(iteration, *current, *candidate, *best, evaluator, penalties, rng_);
        }
        return best;
    }
};
}  // namespace day_constructor
