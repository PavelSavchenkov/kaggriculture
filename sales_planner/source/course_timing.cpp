#include "registry.hpp"
#include "sales_planner/agents/history_course/source/agent.hpp"
#include "physical_state.hpp"
#include <iomanip>

using Agent = kag::agents::history_course::Agent;

int main(int argc, char** argv) {
    if (argc < 5) {
        std::fprintf(stderr, "usage: course_timing opponent seed_start games output [--native-shops] [--trace] [--inventory-guard]\n");
        return 1;
    }
    const std::string opponent = argv[1];
    const uint64_t seed_start = std::stoull(argv[2]);
    const int games = std::stoi(argv[3]);
    bool native = false, trace = false, inventory_guard = false, day_timing = false, delivery_timing = false;
    bool multiple_sales = false, holding_exchange = false;
    for (int i = 5; i < argc; ++i) {
        const std::string option = argv[i];
        if (option == "--native-shops") native = true;
        else if (option == "--trace") trace = true;
        else if (option == "--inventory-guard") inventory_guard = true;
        else if (option == "--day-hold") day_timing = true;
        else if (option == "--delivery-bound") delivery_timing = day_timing = inventory_guard = true;
        else if (option == "--multiple-sales") multiple_sales = delivery_timing = day_timing = inventory_guard = true;
        else if (option == "--holding-exchange") holding_exchange = multiple_sales = delivery_timing = day_timing = inventory_guard = true;
        else std::abort();
    }
    if (games <= 0) std::abort();
    std::ofstream out(argv[4]); if (!out) std::abort();
    out << std::setprecision(12);
    std::ofstream traces;
    if (trace) { traces.open(std::string(argv[4]) + ".trace.jsonl"); if (!traces) std::abort(); }
    for (int game = 0; game < games; ++game) for (int seat = 0; seat < 2; ++seat)
        for (int branch = 0; branch < 3; ++branch) {
            const uint64_t seed = seed_start + game;
            kag::Config config; config.seed = seed;
            std::array<kag::Sim, 2> worlds{kag::Sim(config), kag::Sim(config)};
            std::array<Agent, 2> own{Agent(branch, false), Agent(branch, true, inventory_guard, day_timing, delivery_timing, multiple_sales, holding_exchange)};
            auto rivals = std::array{make_agent(opponent), make_agent(opponent)};
            for (int v = 0; v < 2; ++v) {
                own[v].reset(kag::agent::runtime::make_agent_init(worlds[v], seat));
                rivals[v].reset(kag::agent::runtime::make_agent_init(worlds[v], seat ^ 1));
            }
            kag::agent::DecisionBudget budget; budget.max_expansions = 100000;
            uint64_t shop_rng = seed ^ 0xa37108e62d045fb9ULL;
            std::array<uint8_t, 8> shops;
            for (auto& shop : shops) shop = compositions::random_word(shop_rng) % kag::N_SHOPS;
            int faults[2][2]{}, worker_days[2][2]{}, changed_turns[2]{}, first_change[2]{-1,-1};
            int change_masks[2]{}, history_underestimates = 0, changed_shops = 0;
            double act_seconds[2]{};
            uint64_t hashes[2][2]{{14695981039346656037ULL,14695981039346656037ULL},
                                  {14695981039346656037ULL,14695981039346656037ULL}};
            const auto began = std::chrono::steady_clock::now();
            double last_cash_difference = 0;
            while (!worlds[0].st.done) {
                const int turn = worlds[0].st.step;
                if (worlds[1].st.step != turn || worlds[1].st.done) std::abort();
                bool decision = false;
                for (int v = 0; v < 2; ++v) {
                    auto& sim = worlds[v];
                    if (!native) std::copy_n(shops.begin(), sim.st.n_shops, sim.st.shops);
                    kag::Action actions[2];
                    const auto started = std::chrono::steady_clock::now();
                    const auto obs = kag::agent::runtime::make_observation(sim, seat);
                    const auto previous_decisions = own[v].timing().decisions;
                    own[v].act(obs, budget, actions[seat]);
                    if (trace && v) {
                        sales_planner::Account account; sales_planner::Resources resources;
                        if (!sales_planner::project_resources(obs, actions[seat], account, resources,
                                config.shed_capacity, config.turns_per_day)) std::abort();
                        traces << "{\"kind\":\"market_input\",\"seed\":" << seed << ",\"seat\":" << seat
                            << ",\"branch\":" << branch << ",\"turn\":" << turn << ",\"stock_total\":" << account.total
                            << ",\"stock\":[";
                        for (int i = 0; i < kag::N_ITEMS; ++i) { if (i) traces << ','; traces << account.stock[i]; }
                        traces << "],\"orders\":[";
                        for (int k = 0; k < actions[seat].n_orders; ++k) {
                            if (k) traces << ',';
                            const auto order = actions[seat].orders[k];
                            traces << '[' << int(order.op) << ',' << int(order.item) << ',' << order.n << ']';
                        }
                        traces << "]}\n";
                    }
                    if (v && own[v].timing().decisions > previous_decisions) {
                        decision = true;
                        if (trace) {
                            traces << "{\"kind\":\"decision\",\"seed\":" << seed << ",\"seat\":" << seat
                                << ",\"branch\":" << branch << ",\"turn\":" << turn << ",\"predicted_total\":" << own[v].timing().predicted_gain
                                << ",\"edits\":" << own[v].timing().decisions - previous_decisions
                                << ",\"last_target\":" << own[v].timing().last_target << ",\"last_item\":" << own[v].timing().last_item
                                << ",\"quantity\":" << own[v].timing().last_quantity << ",\"pending\":[";
                            for (int i = 0; i < kag::N_PRODUCTS; ++i) { if (i) traces << ','; traces << own[v].timing().pending[i]; }
                            traces << "],\"prices\":[";
                            for (int i = 0; i < kag::N_PRODUCTS; ++i) { if (i) traces << ','; traces << obs.market.prices[i]; }
                            traces << "]}\n";
                        }
                    }
                    act_seconds[v] += std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
                    if (v) {
                        // Evaluator-only private truth, never supplied to Agent.
                        const auto& rival = sim.st.farms[seat ^ 1];
                        for (int item = 1; item < kag::FERTILIZER; ++item) {
                            int stock = rival.shed[item];
                            for (int u = 0; u < rival.n_units; ++u) stock += rival.inv[u][item];
                            history_underestimates += stock > own[v].history().upper[item];
                        }
                    }
                    const auto other_obs = kag::agent::runtime::make_observation(sim, seat ^ 1);
                    rivals[v].act(other_obs, budget, actions[seat ^ 1]);
                    compositions::validate_action(actions[seat], obs);
                    compositions::validate_action(actions[seat ^ 1], other_obs);
                    const auto diagnostics = sim.diagnose_joint_actions(actions[0], actions[1]);
                    for (int p = 0; p < 2; ++p) {
                        compositions::hash_action(hashes[v][p], actions[p]);
                        faults[v][p] += diagnostics.players[p].requested_unit_actions - diagnostics.players[p].successful_unit_actions;
                        if (sim.st.hour == 23 || (sim.st.day == 29 && sim.st.hour == 22))
                            worker_days[v][p] += sim.st.farms[p].n_units;
                    }
                    sim.step(actions[0], actions[1]);
                }
                for (int side = 0; side < 2; ++side) {
                    const int p = side ? seat ^ 1 : seat;
                    const int difference = sales_planner::physical_difference(worlds[0].st.farms[p], worlds[1].st.farms[p]);
                    if (difference) {
                        ++changed_turns[side]; change_masks[side] |= difference;
                        if (first_change[side] < 0) first_change[side] = turn;
                    }
                }
                changed_shops += worlds[0].st.n_shops != worlds[1].st.n_shops ||
                    !std::equal(worlds[0].st.shops, worlds[0].st.shops + worlds[0].st.n_shops, worlds[1].st.shops);
                const double cash_difference = worlds[1].st.farms[seat].money - worlds[0].st.farms[seat].money;
                if (trace && (decision || cash_difference != last_cash_difference)) {
                    traces << "{\"kind\":\"after_turn\",\"seed\":" << seed << ",\"seat\":" << seat
                        << ",\"branch\":" << branch << ",\"turn\":" << turn << ",\"cash_difference\":" << cash_difference << ",\"market_difference\":[";
                    for (int i = 0; i < kag::N_PRODUCTS; ++i) {
                        if (i) traces << ',';
                        traces << worlds[1].st.market.inventory[i] - worlds[0].st.market.inventory[i];
                    }
                    traces << "]}\n";
                }
                last_cash_difference = cash_difference;
            }
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
            for (int v = 0; v < 2; ++v) {
                const auto& a = worlds[v].st.farms[seat]; const auto& b = worlds[v].st.farms[seat ^ 1];
                out << "{\"seed\":" << seed << ",\"seat\":" << seat << ",\"branch\":" << branch
                    << ",\"opponent\":\"" << opponent << "\",\"variant\":\"" << (v ? "history" : "repair")
                    << "\",\"cash\":" << a.money << ",\"rival_cash\":" << b.money << ",\"margin\":" << a.money - b.money
                    << ",\"own_faults\":" << faults[v][seat] << ",\"rival_faults\":" << faults[v][seat ^ 1]
                    << ",\"worker_days\":" << worker_days[v][seat] << ",\"rival_worker_days\":" << worker_days[v][seat ^ 1]
                    << ",\"own_action_hash\":" << hashes[v][seat] << ",\"rival_action_hash\":" << hashes[v][seat ^ 1]
                    << ",\"own_changed_turns\":" << changed_turns[0] << ",\"rival_changed_turns\":" << changed_turns[1]
                    << ",\"first_own_change\":" << first_change[0] << ",\"first_rival_change\":" << first_change[1]
                    << ",\"own_change_mask\":" << change_masks[0] << ",\"rival_change_mask\":" << change_masks[1]
                    << ",\"shop_changes\":" << changed_shops << ",\"history_underestimates\":" << history_underestimates
                    << ",\"inventory_guard\":" << (v && inventory_guard)
                    << ",\"day_timing\":" << (v && day_timing)
                    << ",\"delivery_timing\":" << (v && delivery_timing)
                    << ",\"multiple_sales\":" << (v && multiple_sales)
                    << ",\"holding_exchange\":" << (v && holding_exchange)
                    << ",\"compiled\":" << own[v].compiled().complete << ",\"compile_faults\":" << own[v].compiled().failed_work
                    << ",\"compile_seconds\":" << own[v].compile_seconds() << ",\"act_seconds\":" << act_seconds[v]
                    << ",\"pair_seconds\":" << seconds << ",\"decisions\":" << own[v].timing().decisions
                    << ",\"delayed_units\":" << own[v].timing().delayed_units << ",\"predicted_gain\":" << own[v].timing().predicted_gain
                    << ",\"holding_exchanges\":" << own[v].timing().holding_exchanges
                    << ",\"repair_decisions\":" << own[v].repairs().decisions;
                auto array = [&](const char* name, const auto* values) {
                    out << ",\"" << name << "\":[";
                    for (int i = 0; i < kag::N_ITEMS; ++i) { if (i) out << ','; out << values[i]; }
                    out << ']';
                };
                array("produced", a.produced); array("rival_produced", b.produced);
                array("stock", a.shed); array("rival_stock", b.shed);
                array("discarded", a.discarded); array("rival_discarded", b.discarded);
                out << "}\n";
            }
            out.flush();
        }
}
