#include "case.hpp"
#include "project_resources.hpp"
#include "rival_stock.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <chrono>

using namespace sales_planner;

int main(int argc, char** argv) {
    require(argc > 1, "usage: check_rival_stock case.calendar [...]");
    for (int file = 1; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        kag::Sim sim(c.config);
        std::array<RivalStockHistory, 2> history;
        int harvest_under = 0, stock_under = 0, sale_mismatch = 0, own_sale_mismatch = 0;
        int observations = 0, exact_stock = 0, true_empty = 0, proved_empty = 0, false_risk = 0, sale_exact = 0;
        double microseconds = 0;
        for (int t = 0; t < int(c.turns.size()); ++t) {
            std::array<kag::agent::AgentObservation, 2> before;
            std::array<Items, 2> sold;
            for (int p = 0; p < 2; ++p) {
                before[p] = kag::agent::runtime::make_observation(sim, p);
                Account own; Resources resources;
                require(project_resources(before[p], c.turns[t].original_actions[p], own, resources), "own projection failed");
                sold[p] = requested_output_sales(own, c.turns[t].original_orders[p]);
            }
            const auto source_before = sim.st;
            sim.step(c.turns[t].original_actions[0], c.turns[t].original_actions[1]);
            for (int p = 0; p < 2; ++p) {
                const auto after = kag::agent::runtime::make_observation(sim, p);
                const auto began = std::chrono::steady_clock::now();
                history[p].observe(before[p], after, sold[p], rules_for(c.config));
                microseconds += std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - began).count();
                const auto& h = history[p];
                const auto& old = source_before.farms[p ^ 1]; const auto& now = sim.st.farms[p ^ 1];
                for (int item = 1; item < kag::FERTILIZER; ++item) {
                    const int produced = now.produced[item] - old.produced[item];
                    const int traded = now.sold_units[item] - old.sold_units[item];
                    const int own_traded = sim.st.farms[p].sold_units[item] - source_before.farms[p].sold_units[item];
                    int actual = now.shed[item];
                    for (int u = 0; u < now.n_units; ++u) actual += now.inv[u][item];
                    harvest_under += h.last_harvest_upper[item] < produced;
                    stock_under += h.upper[item] < actual;
                    sale_mismatch += h.last_sale_exact[item] ? h.last_sale_lower[item] != traded : h.last_sale_lower[item] > traded;
                    own_sale_mismatch += sold[p][item] != own_traded;
                    ++observations; exact_stock += h.upper[item] == actual;
                    true_empty += actual == 0; proved_empty += h.upper[item] == 0;
                    false_risk += actual == 0 && h.upper[item] > 0;
                    sale_exact += h.last_sale_exact[item];
                    if (h.last_harvest_upper[item] < produced || h.upper[item] < actual ||
                        (h.last_sale_exact[item] && h.last_sale_lower[item] != traded))
                        std::fprintf(stderr, "{\"episode\":%llu,\"observer\":%d,\"turn\":%d,\"item\":%d,\"harvest_upper\":%d,\"produced\":%d,\"stock_upper\":%d,\"actual_stock\":%d,\"sale_lower\":%d,\"actual_sale\":%d,\"sale_exact\":%d}\n",
                            (unsigned long long)c.episode, p, t, item, h.last_harvest_upper[item], produced,
                            h.upper[item], actual, h.last_sale_lower[item], traded, h.last_sale_exact[item]);
                }
            }
        }
        std::printf("{\"episode\":%llu,\"observations\":%d,\"harvest_under\":%d,\"stock_under\":%d,\"sale_mismatch\":%d,\"own_sale_mismatch\":%d,\"exact_stock\":%d,\"true_empty\":%d,\"proved_empty\":%d,\"false_risk\":%d,\"exact_sale_observations\":%d,\"microseconds_per_update\":%.4f}\n",
            (unsigned long long)c.episode, observations, harvest_under, stock_under, sale_mismatch, own_sale_mismatch,
            exact_stock, true_empty, proved_empty, false_risk, sale_exact, microseconds / (2 * c.turns.size()));
    }
}
