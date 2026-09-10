#pragma once
#include "storage.hpp"
#include "continuation.hpp"

namespace sales_planner {
struct ValueStats {
    int decisions = 0, accepted = 0, evaluations = 0, evaluated_turns = 0, unknown = 0;
    double predicted_gain = 0;
};

inline Orders valued_storage_scenarios(const PlannerObservation& obs, std::span<const CalendarTurn> calendar,
                                      std::span<const Orders> warm, const MarketRules& rules,
                                      std::span<const MarketForecast> forecasts,
                                      StorageStats& storage, ValueStats& stats, bool require_all_gains = false) {
    if (forecasts.empty() || forecasts.size() > 16) std::abort();
    StorageStats proposal_stats;
    const auto proposal = make_room(obs, calendar, warm[obs.turn], rules, proposal_stats);
    const auto original = compact_orders(obs.own, warm[obs.turn], 1);
    if (!proposal_stats.decisions) return original;
    ++stats.decisions;
    std::array<ContinuationResult, 16> baselines;
    for (int i = 0; i < int(forecasts.size()); ++i) {
        baselines[i] = evaluate_continuation(obs, calendar, warm, forecasts[i], rules, &original);
        ++stats.evaluations; stats.evaluated_turns += baselines[i].evaluated_turns;
        if (!baselines[i].feasible()) { ++stats.unknown; return original; }
    }
    Orders best = original;
    double best_gain = 0;
    int best_quantity = 0;
    const auto sale = proposal.values[proposal.count - 1];
    for (int q = 1; q <= sale.n; ++q) {
        auto candidate = proposal; candidate.values[candidate.count - 1].n = q;
        double gain = 0; bool valid = true;
        for (int i = 0; i < int(forecasts.size()); ++i) {
            const auto result = evaluate_continuation(obs, calendar, warm, forecasts[i], rules, &candidate);
            ++stats.evaluations; stats.evaluated_turns += result.evaluated_turns;
            const double delta = result.margin() - baselines[i].margin();
            if (!result.feasible() || !keeps_ending_resources(result, baselines[i]) ||
                (require_all_gains && delta < 0)) { valid = false; break; }
            gain += delta;
        }
        if (!valid) continue;
        gain /= forecasts.size();
        if (gain <= best_gain) continue;
        best_gain = gain; best = candidate; best_quantity = q;
    }
    if (best_quantity) {
        ++stats.accepted; stats.predicted_gain += best_gain;
        ++storage.decisions; storage.sold += best_quantity;
    }
    return best;
}

// First forecast: keep currently known shops and assume no rival trades.
inline Orders valued_storage(const PlannerObservation& obs, std::span<const CalendarTurn> calendar,
                             std::span<const Orders> warm, const MarketRules& rules,
                             StorageStats& storage, ValueStats& stats) {
    MarketForecast forecast; forecast.rival.cash = obs.rival_cash;
    return valued_storage_scenarios(obs, calendar, warm, rules,
                                   std::span<const MarketForecast>(&forecast, 1), storage, stats);
}
}
