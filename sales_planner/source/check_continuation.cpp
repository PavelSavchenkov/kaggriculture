#include "case.hpp"
#include "continuation.hpp"

using namespace sales_planner;
int main(int argc, char** argv) {
    require(argc > 1, "usage: check_continuation case.calendar [...]");
    for (int file = 1; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        for (int seat = 0; seat < 2; ++seat) {
            std::vector<CalendarTurn> own_calendar, rival_calendar;
            std::vector<Orders> own_orders, rival_orders;
            for (const auto& t : c.turns) {
                own_calendar.push_back(t.calendar[seat]); rival_calendar.push_back(t.calendar[seat ^ 1]);
                own_orders.push_back(t.original_orders[seat]); rival_orders.push_back(t.original_orders[seat ^ 1]);
            }
            PlannerObservation start;
            start.own = c.initial.financial.accounts[seat]; start.resources = c.initial.resources[seat];
            start.inventory = c.initial.financial.inventory;
            start.shops = c.turns[0].shops; start.n_shops = c.turns[0].n_shops;
            start.rival_cash = c.initial.financial.accounts[seat ^ 1].cash;
            apply(start.own, start.resources, own_calendar[0].before_market, c.config.shed_capacity);
            MarketForecast world;
            world.rival = c.initial.financial.accounts[seat ^ 1]; world.rival_resources = c.initial.resources[seat ^ 1];
            world.rival_calendar = rival_calendar; world.rival_orders = rival_orders;
            for (int t = 1; t < int(c.turns.size()); ++t)
                for (int k = c.turns[t - 1].n_shops; k < c.turns[t].n_shops; ++k)
                    world.arrivals[world.n_arrivals++] = {t, c.turns[t].shops[k]};
            const auto result = evaluate_continuation(start, own_calendar, own_orders, world, rules_for(c.config));
            std::printf("{\"episode\":%llu,\"seat\":%d,\"cash\":%.0f,\"rival_cash\":%.0f,\"feasible\":%s,\"turns\":%d}\n",
                        (unsigned long long)c.episode, seat, result.accounts[0].cash, result.accounts[1].cash,
                        result.feasible() ? "true" : "false", result.evaluated_turns);
        }
    }
}
