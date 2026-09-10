#include "case.hpp"
#include "continuation.hpp"
#include <chrono>

using namespace sales_planner;

int main(int argc, char** argv) {
    require(argc > 2, "usage: benchmark_continuation repetitions case.calendar [...]");
    const int repetitions = std::stoi(argv[1]); require(repetitions > 0, "invalid repetitions");
    struct Input {
        PlannerObservation start;
        MarketRules rules;
        std::vector<CalendarTurn> own, rival;
        std::vector<Orders> orders, rival_orders;
        MarketForecast world;
    };
    std::vector<Input> inputs;
    inputs.reserve((argc - 2) * 2);
    for (int file = 2; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        for (int seat = 0; seat < 2; ++seat) {
            auto& x = inputs.emplace_back(); x.rules = rules_for(c.config);
            for (const auto& t : c.turns) {
                x.own.push_back(t.calendar[seat]); x.rival.push_back(t.calendar[seat ^ 1]);
                x.orders.push_back(t.original_orders[seat]); x.rival_orders.push_back(t.original_orders[seat ^ 1]);
            }
            x.start.own = c.initial.financial.accounts[seat]; x.start.resources = c.initial.resources[seat];
            x.start.inventory = c.initial.financial.inventory;
            x.start.shops = c.turns[0].shops; x.start.n_shops = c.turns[0].n_shops;
            x.start.rival_cash = c.initial.financial.accounts[seat ^ 1].cash;
            apply(x.start.own, x.start.resources, x.own[0].before_market, x.rules.capacity);
            x.world.rival = c.initial.financial.accounts[seat ^ 1]; x.world.rival_resources = c.initial.resources[seat ^ 1];
            x.world.rival_calendar = x.rival; x.world.rival_orders = x.rival_orders;
            for (int t = 1; t < int(c.turns.size()); ++t)
                for (int k = c.turns[t - 1].n_shops; k < c.turns[t].n_shops; ++k)
                    x.world.arrivals[x.world.n_arrivals++] = {t, c.turns[t].shops[k]};
        }
    }
    double checksum = 0;
    for (const auto& x : inputs) checksum += evaluate_continuation(x.start, x.own, x.orders, x.world, x.rules).margin();
    const auto begin = std::chrono::steady_clock::now();
    for (int repeat = 0; repeat < repetitions; ++repeat)
        for (const auto& x : inputs) {
            const auto value = evaluate_continuation(x.start, x.own, x.orders, x.world, x.rules);
            checksum += value.margin();
            for (int side = 0; side < 2; ++side) checksum += value.resources[side].missing[kag::WHEAT];
        }
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    std::printf("{\"calls\":%llu,\"seconds\":%.9f,\"microseconds_per_call\":%.6f,\"checksum\":%.0f}\n",
        (unsigned long long)(inputs.size() * repetitions), seconds, seconds * 1e6 / (inputs.size() * repetitions), checksum);
}
