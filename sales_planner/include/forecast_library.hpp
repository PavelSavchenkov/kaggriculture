#pragma once
#include "case.hpp"
#include "continuation.hpp"

namespace sales_planner {
enum class RivalCashSource { current_observation, historical_case };
// Offline-supplied scenario library. Public historical private stock is a
// possible rival state, not knowledge of the present rival's private stock.
// The supplied scenario remains conditional if its fixed production would need
// a different physical farm. Source and evaluation episodes must be separated.
struct HistoricalScenario {
    std::vector<Account> accounts;
    std::vector<Resources> resources;
    std::vector<CalendarTurn> calendar;
    std::vector<Orders> orders;
    std::vector<std::array<uint8_t, 8>> shops;
    std::vector<int> shop_counts;
    uint64_t source_episode = 0;
    int source_seat = 0;

    HistoricalScenario(const Case& c, int seat) : source_episode(c.episode), source_seat(seat) {
        for (int t = 0; t < int(c.turns.size()); ++t) {
            const auto& before = t ? c.turns[t - 1].expected : c.initial;
            accounts.push_back(before.financial.accounts[seat]);
            resources.push_back(before.resources[seat]);
            calendar.push_back(c.turns[t].calendar[seat]); orders.push_back(c.turns[t].original_orders[seat]);
            shops.push_back(c.turns[t].shops); shop_counts.push_back(c.turns[t].n_shops);
        }
    }

    MarketForecast at(const PlannerObservation& current,
                      RivalCashSource cash_source = RivalCashSource::current_observation) const {
        MarketForecast result;
        result.rival = accounts[current.turn];
        if (cash_source == RivalCashSource::current_observation) result.rival.cash = current.rival_cash;
        // Historical cash defines a different stress case. It is not extra
        // funding granted to the observed rival, nor proof of compatibility
        // with the observed rival's public farm or private resources.
        result.rival_resources = resources[current.turn];
        result.rival_calendar = calendar; result.rival_orders = orders;
        for (int t = current.turn + 1; t < int(calendar.size()); ++t)
            for (int k = shop_counts[t - 1]; k < shop_counts[t]; ++k)
                result.arrivals[result.n_arrivals++] = {t, shops[t][k]};
        return result;
    }
};
}
