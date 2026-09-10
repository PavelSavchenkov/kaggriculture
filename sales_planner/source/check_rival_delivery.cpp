#include "case.hpp"
#include "project_resources.hpp"
#include "rival_delivery.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <chrono>

using namespace sales_planner;

int main(int argc, char** argv) {
    require(argc > 1, "usage: check_rival_delivery case.calendar [...]");
    for (int file = 1; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        kag::Sim sim(c.config);
        std::array<RivalStockHistory, 2> history;
        std::array<std::array<int, kag::N_PRODUCTS>, 2> promised{};
        int observations = 0, violations = 0, stock_under = 0, extra_windows = 0, sale_events = 0;
        int extra_window_turns = 0;
        double microseconds = 0;
        for (int t = 0; t < int(c.turns.size()); ++t) {
            std::array<kag::agent::AgentObservation, 2> before;
            std::array<Items, 2> sold;
            for (int p = 0; p < 2; ++p) {
                before[p] = kag::agent::runtime::make_observation(sim, p);
                const auto began = std::chrono::steady_clock::now();
                const auto bound = rival_sale_not_before(before[p], history[p], rules_for(c.config));
                microseconds += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - began).count();
                std::array<int, kag::N_PRODUCTS> ready{};
                for (const auto& row : before[p].opponent().tiles) for (const auto& tile : row) {
                    if (tile.has_animal) ready[kag::ANIMALS[tile.what - kag::GOOSE].product] += tile.yield_units;
                    else if (tile.kind == kag::T_PLANT && before[p].day - tile.planted_day >= kag::CROPS[tile.what].first_yield_day)
                        ready[tile.what] += tile.yield_units;
                }
                for (int item = 1; item < kag::FERTILIZER; ++item) {
                    ++observations;
                    promised[p][item] = std::max(promised[p][item], bound[item]);
                    if (ready[item] && !history[p].upper[item] && bound[item] > t + 1) {
                        ++extra_windows; extra_window_turns += bound[item] - t - 1;
                    }
                }
                Account own; Resources resources;
                require(project_resources(before[p], c.turns[t].original_actions[p], own, resources), "own projection failed");
                sold[p] = requested_output_sales(own, c.turns[t].original_orders[p]);
            }
            const auto source_before = sim.st;
            sim.step(c.turns[t].original_actions[0], c.turns[t].original_actions[1]);
            for (int p = 0; p < 2; ++p) {
                const auto after = kag::agent::runtime::make_observation(sim, p);
                history[p].observe(before[p], after, sold[p], rules_for(c.config));
                const auto& old = source_before.farms[p ^ 1]; const auto& now = sim.st.farms[p ^ 1];
                for (int item = 1; item < kag::FERTILIZER; ++item) {
                    const int traded = now.sold_units[item] - old.sold_units[item];
                    int actual = now.shed[item];
                    for (int u = 0; u < now.n_units; ++u) actual += now.inv[u][item];
                    stock_under += history[p].upper[item] < actual;
                    sale_events += traded > 0;
                    if (traded > 0 && t < promised[p][item]) {
                        ++violations;
                        std::fprintf(stderr, "{\"episode\":%llu,\"observer\":%d,\"turn\":%d,\"item\":%d,\"not_before\":%d,\"sold\":%d}\n",
                            (unsigned long long)c.episode, p, t, item, promised[p][item], traded);
                    }
                }
            }
        }
        std::printf("{\"episode\":%llu,\"observations\":%d,\"sale_events\":%d,\"violations\":%d,\"stock_under\":%d,\"extra_windows\":%d,\"extra_window_turns\":%d,\"microseconds_per_call\":%.4f}\n",
            (unsigned long long)c.episode, observations, sale_events, violations, stock_under, extra_windows, extra_window_turns,
            microseconds / (2 * c.turns.size()));
    }
}
