#include "case.hpp"
#include "storage.hpp"
#include "value_storage.hpp"
#include "forecast_library.hpp"
#include "project_resources.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <chrono>

using namespace sales_planner;

int main(int argc, char** argv) {
    require(argc > 1, "usage: replay_storage case.calendar [...]");
    const bool trace = std::string(argv[1]) == "--trace";
    const char* names[] = {"original", "compact_sales", "room_keep", "room_all", "room_value_quiet", "room_value_scenarios", "room_value_all_gains"};
    int first_file = trace ? 2 : 1;
    std::vector<HistoricalScenario> library;
    if (std::string(argv[1]) == "--scenarios") {
        require(argc > 3, "missing scenario paths");
        const int count = std::stoi(argv[2]);
        require(count > 0 && count <= 8 && argc > count + 3, "invalid scenario count");
        for (int i = 0; i < count; ++i) {
            auto source = read_case(argv[i + 3]); certify_commitments(source);
            for (int seat = 0; seat < 2; ++seat) library.emplace_back(source, seat);
        }
        first_file = count + 3;
    }
    for (int file = first_file; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        for (const auto& source : library) require(source.source_episode != c.episode, "scenario source overlaps evaluation");
        for (int seat = 0; seat < 2; ++seat) {
            std::vector<CalendarTurn> calendar;
            std::vector<Orders> warm;
            for (const auto& turn : c.turns) { calendar.push_back(turn.calendar[seat]); warm.push_back(turn.original_orders[seat]); }
            for (int policy = 0; policy < (library.empty() ? 5 : 7); ++policy) {
                kag::Sim sim(c.config); StorageStats stats; ValueStats value;
                int faults[2]{}, requested[2]{}, changed = 0;
                const auto started = std::chrono::steady_clock::now();
                for (int t = 0; t < int(c.turns.size()); ++t) {
                    const int previous_decisions = stats.decisions;
                    const auto& turn = c.turns[t];
                    auto actions = turn.original_actions;
                    if (policy) {
                        const auto observed = kag::agent::runtime::make_observation(sim, seat);
                        PlannerObservation obs; obs.turn = t;
                        require(project_resources(observed, actions[seat], obs.own, obs.resources,
                                                  c.config.shed_capacity, c.config.turns_per_day), "projection unsupported");
                        std::copy_n(observed.market.inventory, kag::N_PRODUCTS, obs.inventory.begin());
                        std::copy_n(observed.shops, observed.n_shops, obs.shops.begin());
                        obs.n_shops = observed.n_shops; obs.rival_cash = observed.opponent().money;
                        Orders orders;
                        const auto rules = rules_for(c.config);
                        if (policy == 1) orders = compact_orders(obs.own, turn.original_orders[seat], 1);
                        else if (policy <= 3) orders = make_room(obs, calendar, turn.original_orders[seat], rules, stats,
                                                                policy == 3 ? terminal_turn : 0);
                        else if (policy == 4) orders = valued_storage(obs, calendar, warm, rules, stats, value);
                        else {
                            StorageStats preflight;
                            orders = make_room(obs, calendar, turn.original_orders[seat], rules, preflight);
                            if (preflight.decisions) {
                                std::array<MarketForecast, 16> forecasts;
                                for (int k = 0; k < int(library.size()); ++k) forecasts[k] = library[k].at(obs);
                                orders = valued_storage_scenarios(obs, calendar, warm, rules,
                                    std::span<const MarketForecast>(forecasts.data(), library.size()), stats, value, policy == 6);
                            }
                        }
                        bool difference = orders.count != actions[seat].n_orders;
                        for (int k = 0; k < orders.count && !difference; ++k) {
                            const auto a = orders.values[k], b = actions[seat].orders[k];
                            difference = a.op != b.op || a.item != b.item || a.n != b.n;
                        }
                        changed += difference;
                        actions[seat].n_orders = orders.count;
                        std::copy_n(orders.values.begin(), orders.count, actions[seat].orders);
                        actions[seat].finalize();
                    }
                    const auto outcome = sim.diagnose_joint_actions(actions[0], actions[1]);
                    for (int p = 0; p < 2; ++p) {
                        requested[p] += outcome.players[p].requested_unit_actions;
                        faults[p] += outcome.players[p].requested_unit_actions - outcome.players[p].successful_unit_actions;
                    }
                    sim.step(actions[0], actions[1]);
                    if (trace && stats.decisions > previous_decisions) {
                        const auto o = actions[seat].orders[actions[seat].n_orders - 1];
                        std::fprintf(stderr, "episode=%llu seat=%d policy=%s turn=%d item=%d quantity=%d cash=%.0f rival_cash=%.0f inventory=%d\n",
                                     (unsigned long long)c.episode, seat, names[policy], t, o.item, o.n,
                                     sim.st.farms[seat].money, sim.st.farms[seat ^ 1].money, sim.st.market.inventory[o.item]);
                    }
                }
                const auto& a = sim.st.farms[seat]; const auto& b = sim.st.farms[seat ^ 1];
                std::printf("{\"episode\":%llu,\"seat\":%d,\"policy\":\"%s\",\"cash\":%.0f,\"rival_cash\":%.0f,\"margin\":%.0f,\"own_faults\":%d,\"rival_faults\":%d,\"own_requested\":%d,\"rival_requested\":%d,\"changed\":%d,\"storage_decisions\":%d,\"storage_sold\":%d,\"own_produced\":[",
                            (unsigned long long)c.episode, seat, names[policy], a.money, b.money, a.money - b.money,
                            faults[seat], faults[seat ^ 1], requested[seat], requested[seat ^ 1], changed, stats.decisions, stats.sold);
                for (int i = 0; i < kag::N_ITEMS; ++i) std::printf("%s%d", i ? "," : "", a.produced[i]);
                std::printf("],\"rival_produced\":[");
                for (int i = 0; i < kag::N_ITEMS; ++i) std::printf("%s%d", i ? "," : "", b.produced[i]);
                std::printf("],\"own_discarded\":[");
                for (int i = 0; i < kag::N_ITEMS; ++i) std::printf("%s%d", i ? "," : "", a.discarded[i]);
                std::printf("],\"seconds\":%.6f}\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
            }
        }
    }
}
