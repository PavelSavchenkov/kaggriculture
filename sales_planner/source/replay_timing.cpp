#include "case.hpp"
#include "timing.hpp"
#include "day_timing.hpp"
#include "project_resources.hpp"
#include "rival_stock.hpp"
#include "rival_delivery.hpp"
#include "seed_budget.hpp"
#include "holding_exchange.hpp"
#include "seed_prebuy.hpp"
#include "evaluation.hpp"
#include <tuple>

using namespace sales_planner;

static auto tile_fields(const kag::Tile& t) {
    return std::tie(t.kind, t.what, t.has_animal, t.watered_today, t.fed_today,
        t.cared_today, t.fertilizer_available, t.consecutive_dry, t.yield_units,
        t.pending_care_bonus, t.planted_day, t.max_lifespan_step, t.fertilized_until_day);
}

// Compare the physical plan every turn; shed stock is allowed to differ while
// a sale waits. Cash and trade counters are deliberately outside this check.
static int work_difference(const kag::Farm& a, const kag::Farm& b) {
    int mask = 0;
    if (a.n_units != b.n_units || a.n_quadrants != b.n_quadrants || a.hires_today != b.hires_today) mask |= 1;
    if (!std::equal(a.seeds, a.seeds + kag::N_CROPS, b.seeds)) mask |= 2;
    for (int y = 0; y < kag::BOARD; ++y)
        for (int x = 0; x < kag::BOARD; ++x)
            if (tile_fields(a.tiles[y][x]) != tile_fields(b.tiles[y][x])) mask |= 4;
    for (int u = 0; u < std::min(a.n_units, b.n_units); ++u) {
        if (a.pos_x[u] != b.pos_x[u] || a.pos_y[u] != b.pos_y[u]) mask |= 8;
        if (!std::equal(a.inv[u], a.inv[u] + kag::N_ITEMS, b.inv[u])) mask |= 16;
        if (a.inv_nkeys[u] != b.inv_nkeys[u] ||
            !std::equal(a.inv_keys[u], a.inv_keys[u] + a.inv_nkeys[u], b.inv_keys[u])) mask |= 32;
    }
    return mask;
}

static std::array<int, kag::N_PRODUCTS> ready(const kag::agent::AgentObservation& o) {
    std::array<int, kag::N_PRODUCTS> result{};
    for (const auto& row : o.opponent().tiles)
        for (const auto& tile : row) {
            if (tile.has_animal) result[kag::ANIMALS[tile.what - kag::GOOSE].product] += tile.yield_units;
            else if (tile.kind == kag::T_PLANT && o.day - tile.planted_day >= kag::CROPS[tile.what].first_yield_day)
                result[tile.what] += tile.yield_units;
        }
    return result;
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    require(argc > 1, "usage: replay_timing [--mixed] [--guarded] [--source-actions] [--history|--history-recent] case.calendar [...]");
    const char* names[] = {"original", "wait_demand", "wait_no_ready"};
    bool mixed = false, guarded = false, source_actions = false;
    bool history_mode = false, history_recent = false;
    bool day_mode = false, inventory_guard = false, delivery_mode = false, seed_cap = false, seed_restock = false;
    bool purchase_witness = false, multiple_sales = false, holding_exchange = false;
    bool slot_fill = false, seed_prebuy = false;
    bool outputs_only = false;
    int selected_seat = -1;
    int first_file = 1;
    while (first_file < argc && std::string(argv[first_file]).starts_with("--")) {
        const std::string option = argv[first_file++];
        if (option == "--mixed") mixed = true;
        else if (option == "--guarded") guarded = true;
        else if (option == "--source-actions") source_actions = true;
        else if (option == "--inventory-guard") inventory_guard = true;
        else if (option == "--day-hold") day_mode = history_mode = mixed = guarded = inventory_guard = true;
        else if (option == "--delivery-bound") delivery_mode = day_mode = history_mode = mixed = guarded = inventory_guard = true;
        else if (option == "--seed-cap") seed_cap = true;
        else if (option == "--seed-restock") seed_cap = seed_restock = true;
        else if (option == "--purchase-witness") purchase_witness = true;
        else if (option == "--multiple-sales") multiple_sales = true;
        else if (option == "--holding-exchange") holding_exchange = multiple_sales = true;
        else if (option == "--slot-fill") slot_fill = multiple_sales = true;
        else if (option == "--seed-prebuy") seed_prebuy = slot_fill = multiple_sales = true;
        else if (option == "--outputs") outputs_only = mixed = guarded = true;
        else if (option == "--seat0") selected_seat = 0;
        else if (option == "--seat1") selected_seat = 1;
        else if (option == "--history" || option == "--history-recent") {
            history_mode = mixed = guarded = true; history_recent = option == "--history-recent";
        }
        else require(false, "unknown timing replay option");
    }
    require(first_file < argc, "missing replay cases");
    if (mixed) { names[1] = "wait_mixed_demand"; names[2] = "wait_mixed_no_ready"; }
    if (outputs_only) names[2] = "wait_output_recent";
    if (history_mode) names[2] = history_recent ? "wait_history_recent" : "wait_history";
    if (history_mode && inventory_guard) names[2] = "wait_history_floor";
    if (day_mode) names[2] = "wait_day_history";
    if (delivery_mode) names[2] = "wait_delivery_history";
    require(!seed_cap || delivery_mode, "seed cap currently requires delivery timing");
    if (seed_cap) names[2] = "wait_delivery_seedcap";
    if (seed_restock) names[2] = "wait_delivery_restock";
    require(!purchase_witness || (delivery_mode && !seed_cap), "purchase witness requires unmodified delivery rule");
    if (purchase_witness) names[2] = "wait_delivery_witness";
    require(!multiple_sales || (delivery_mode && !seed_cap), "multiple sales requires delivery timing");
    if (multiple_sales) names[2] = purchase_witness ? "wait_delivery_multiple_witness" : "wait_delivery_multiple";
    if (holding_exchange) names[2] = purchase_witness ? "wait_delivery_exchange_witness" : "wait_delivery_exchange";
    require(!slot_fill || (purchase_witness && !holding_exchange), "sale slot test requires supplied purchase quantities");
    if (slot_fill) names[2] = seed_prebuy ? "wait_delivery_seedprebuy_witness" : "wait_delivery_slotfill_witness";
    for (int file = first_file; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        const auto rules = rules_for(c.config);
        for (int seat = 0; seat < 2; ++seat) {
            if (selected_seat >= 0 && seat != selected_seat) continue;
            std::vector<CalendarTurn> calendar;
            std::vector<Orders> warm_orders;
            for (const auto& turn : c.turns) {
                calendar.push_back(turn.calendar[seat]);
                warm_orders.push_back(purchase_witness ? turn.purchase_witness[seat] : turn.original_orders[seat]);
            }
            const SeedBudget seed_budget(calendar, c.turns.back().expected.financial.accounts[seat].seeds,
                seed_restock ? std::span<const Orders>(warm_orders) : std::span<const Orders>{});
            for (int policy = 0; policy < 3; ++policy) {
                if (guarded && policy == 1) continue;
                kag::Sim sim(c.config), reference(c.config);
                TimingMemory memory;
                RivalStockHistory history;
                SeedBudgetStats seed_stats;
                SaleSlotStats slot_stats;
                std::vector<Orders> order_plan;
                if (day_mode) order_plan = warm_orders;
                int faults[2]{}, requested[2]{}, work_differences[2]{}, work_masks[2]{}, changed = 0, shop_differences = 0;
                int source_contract_exceptions[2]{};
                int history_underestimates = 0;
                int event_turn = -2, event_item = -1, event_quantity = 0;
                double event_cash = 0, event_rival_cash = 0, event_prediction = 0;
                const auto started = std::chrono::steady_clock::now();
                for (int t = 0; t < int(c.turns.size()); ++t) {
                    const auto& turn = c.turns[t];
                    auto actions = turn.original_actions;
                    const auto observed = kag::agent::runtime::make_observation(sim, seat);
                    Items own_sold{};
                    if (policy) {
                        PlannerObservation obs; obs.turn = t;
                        require(project_resources(observed, actions[seat], obs.own, obs.resources,
                            rules.capacity, rules.turns_per_day), "resource projection unsupported");
                        std::copy_n(observed.market.inventory, kag::N_PRODUCTS, obs.inventory.begin());
                        std::copy_n(observed.shops, observed.n_shops, obs.shops.begin());
                        obs.n_shops = observed.n_shops; obs.rival_cash = observed.opponent().money;
                        const auto decisions = memory.decisions;
                        const double predicted = memory.predicted_gain;
                        const Orders next = t + 1 < int(c.turns.size()) ? c.turns[t + 1].original_orders[seat] : Orders{};
                        uint16_t blocked = 0;
                        if (history_mode || outputs_only)
                            for (int item = 0; item < kag::N_PRODUCTS; ++item)
                                if (!output_product(item) || (history_mode && history.upper[item] > 0)) blocked |= uint16_t{1} << item;
                        std::array<int, kag::N_PRODUCTS> not_before{};
                        if (delivery_mode) not_before = rival_sale_not_before(observed, history, rules);
                        if (seed_cap) order_plan[t] = seed_budget.cap(t, obs.own, order_plan[t], seed_stats);
                        const auto simulations_before = memory.simulations;
                        const auto slot_simulations_before = slot_stats.simulations;
                        if (slot_fill) fill_sale_slot(obs, calendar, order_plan, not_before, slot_stats, rules, 1000);
                        const auto day_planner = holding_exchange ? improve_sale_holding : multiple_sales ? delay_sales_within_day : delay_within_day;
                        auto orders = day_mode ? day_planner(obs, calendar, order_plan,
                            ready(observed), blocked, memory, rules, 1000 - (slot_stats.simulations - slot_simulations_before),
                            delivery_mode ? &not_before : nullptr) :
                            delay_for_demand(obs, calendar, turn.original_orders[seat], next,
                                ready(observed), memory, rules, policy == 2, 20, mixed, blocked,
                                history_mode && !history_recent, inventory_guard);
                        if (seed_prebuy) {
                            const auto used = memory.simulations - simulations_before + slot_stats.simulations - slot_simulations_before;
                            require(used <= 1000, "sale slot work budget exceeded");
                            prebuy_seed_order(obs, calendar, order_plan, slot_stats, rules, 1000 - used);
                            orders = order_plan[t];
                        }
                        if (day_mode) require(memory.simulations - simulations_before + slot_stats.simulations - slot_simulations_before <= 1000,
                            "day work budget exceeded");
                        if (history_mode) own_sold = requested_output_sales(obs.own, orders, rules.max_orders);
                        if (memory.decisions != decisions) {
                            event_turn = t;
                            event_prediction = memory.predicted_gain - predicted;
                            event_cash = sim.st.farms[seat].money - reference.st.farms[seat].money;
                            event_rival_cash = sim.st.farms[seat ^ 1].money - reference.st.farms[seat ^ 1].money;
                            event_item = memory.last_item; event_quantity = memory.last_quantity;
                            if (day_mode && multiple_sales)
                                std::fprintf(stderr, "{\"kind\":\"day_decisions\",\"episode\":%llu,\"seat\":%d,\"turn\":%d,\"edits\":%llu,\"last_target\":%d,\"last_item\":%d,\"last_quantity\":%d,\"predicted_increment\":%.0f}\n",
                                    (unsigned long long)c.episode, seat, t, (unsigned long long)(memory.decisions - decisions),
                                    memory.last_target, event_item, event_quantity, event_prediction);
                            else if (day_mode)
                                std::fprintf(stderr, "{\"kind\":\"day_decision\",\"episode\":%llu,\"seat\":%d,\"turn\":%d,\"target\":%d,\"item\":%d,\"quantity\":%d,\"predicted_increment\":%.0f}\n",
                                    (unsigned long long)c.episode, seat, t, memory.last_target,
                                    event_item, event_quantity, event_prediction);
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
                    } else if (history_mode) {
                        Account own; Resources resources;
                        require(project_resources(observed, actions[seat], own, resources, rules.capacity, rules.turns_per_day),
                            "history own projection failed");
                        own_sold = requested_output_sales(own, turn.original_orders[seat], rules.max_orders);
                    }
                    for (int p = 0; p < 2; ++p) {
                        bool valid = actions[p].n_units == sim.st.farms[p].n_units;
                        for (int u = 0; u < actions[p].n_units; ++u) valid &= actions[p].units[u].op < kag::OP_INVALID;
                        if (!valid) {
                            // Some raw public replays request actions for inactive
                            // workers. The game accepts them; the local agent API
                            // does not. Preserve their exact source semantics and
                            // report this exception only when explicitly requested.
                            const auto& original = turn.original_actions[p];
                            bool unchanged = actions[p].n_units == original.n_units &&
                                sim.st.farms[p].n_units == reference.st.farms[p].n_units;
                            for (int u = 0; u < actions[p].n_units && unchanged; ++u)
                                unchanged = actions[p].units[u].op == original.units[u].op &&
                                    actions[p].units[u].arg == original.units[u].arg && actions[p].units[u].n == original.units[u].n;
                            if (source_actions && unchanged) {
                                ++source_contract_exceptions[p];
                                require(actions[p].metadata_ready && actions[p].n_orders <= rules.max_orders,
                                    "source action market contract changed");
                                continue;
                            }
                            std::fprintf(stderr, "{\"episode\":%llu,\"seat\":%d,\"policy\":\"%s\",\"turn\":%d,\"player\":%d,\"error\":\"action_contract\",\"requested_units\":%d,\"actual_units\":%d}\n",
                                (unsigned long long)c.episode, seat, names[policy], t, p, actions[p].n_units, sim.st.farms[p].n_units);
                            std::exit(1);
                        }
                        compositions::validate_action(actions[p], kag::agent::runtime::make_observation(sim, p));
                    }
                    const auto outcome = sim.diagnose_joint_actions(actions[0], actions[1]);
                    for (int p = 0; p < 2; ++p) {
                        requested[p] += outcome.players[p].requested_unit_actions;
                        faults[p] += outcome.players[p].requested_unit_actions - outcome.players[p].successful_unit_actions;
                    }
                    sim.step(actions[0], actions[1]);
                    if (history_mode) {
                        history.observe(observed, kag::agent::runtime::make_observation(sim, seat), own_sold, rules);
                        // Evaluator-only truth check; these private quantities
                        // never enter the history signal or the timing policy.
                        const auto& rival_truth = sim.st.farms[seat ^ 1];
                        for (int item = 1; item < kag::FERTILIZER; ++item) {
                            int actual_stock = rival_truth.shed[item];
                            for (int u = 0; u < rival_truth.n_units; ++u) actual_stock += rival_truth.inv[u][item];
                            history_underestimates += history.upper[item] < actual_stock;
                        }
                    }
                    reference.step(turn.original_actions[0], turn.original_actions[1]);
                    for (int p = 0; p < 2; ++p) {
                        require(reference.st.farms[p].money == turn.expected.financial.accounts[p].cash,
                            "source cash parity failed");
                        const int difference = work_difference(sim.st.farms[p], reference.st.farms[p]);
                        if (difference && !work_differences[p])
                            std::fprintf(stderr, "{\"episode\":%llu,\"seat\":%d,\"policy\":\"%s\",\"turn\":%d,\"player\":%d,\"error\":\"physical_difference\",\"mask\":%d}\n",
                                (unsigned long long)c.episode, seat, names[policy], t, p, difference);
                        work_differences[p] += difference != 0;
                        work_masks[p] |= difference;
                    }
                    shop_differences += sim.st.n_shops != reference.st.n_shops ||
                        !std::equal(sim.st.shops, sim.st.shops + sim.st.n_shops, reference.st.shops);
                    if (!day_mode && t == event_turn + 1)
                        std::fprintf(stderr, "{\"episode\":%llu,\"seat\":%d,\"policy\":\"%s\",\"turn\":%d,\"item\":%d,\"quantity\":%d,\"predicted_own_gain\":%.0f,\"realized_two_turn_own_gain\":%.0f,\"realized_two_turn_rival_gain\":%.0f}\n",
                            (unsigned long long)c.episode, seat, names[policy], event_turn, event_item, event_quantity, event_prediction,
                            sim.st.farms[seat].money - reference.st.farms[seat].money - event_cash,
                            sim.st.farms[seat ^ 1].money - reference.st.farms[seat ^ 1].money - event_rival_cash);
                }
                const auto& a = sim.st.farms[seat]; const auto& b = sim.st.farms[seat ^ 1];
                std::printf("{\"episode\":%llu,\"seat\":%d,\"policy\":\"%s\",\"cash\":%.0f,\"rival_cash\":%.0f,\"margin\":%.0f,\"own_faults\":%d,\"rival_faults\":%d,\"own_requested\":%d,\"rival_requested\":%d,\"own_work_differences\":%d,\"rival_work_differences\":%d,\"shop_differences\":%d,\"changed\":%d,\"delay_decisions\":%llu,\"delayed_units\":%llu,\"predicted_own_gain\":%.0f",
                    (unsigned long long)c.episode, seat, names[policy], a.money, b.money, a.money - b.money,
                    faults[seat], faults[seat ^ 1], requested[seat], requested[seat ^ 1], work_differences[seat],
                    work_differences[seat ^ 1], shop_differences, changed, (unsigned long long)memory.decisions,
                    (unsigned long long)memory.delayed_units, memory.predicted_gain + slot_stats.predicted_gain);
                std::printf(",\"own_source_contract_exceptions\":%d,\"rival_source_contract_exceptions\":%d",
                    source_contract_exceptions[seat], source_contract_exceptions[seat ^ 1]);
                std::printf(",\"own_work_mask\":%d,\"rival_work_mask\":%d,\"seed_orders_changed\":%d,\"seed_units_removed\":%d",
                    work_masks[seat], work_masks[seat ^ 1], seed_stats.orders_changed, seed_stats.units_removed);
                auto seeds = [&](const char* name, int p) {
                    std::printf(",\"%s\":[", name);
                    for (int i = 0; i < kag::N_CROPS; ++i) std::printf("%s%d", i ? "," : "", sim.st.farms[p].seeds[i]);
                    std::printf("]");
                };
                seeds("own_final_seeds", seat); seeds("rival_final_seeds", seat ^ 1);
                std::printf(",\"history_underestimates\":%d", history_underestimates);
                std::printf(",\"holding_exchanges\":%llu", (unsigned long long)memory.holding_exchanges);
                std::printf(",\"sale_advances\":%llu,\"advanced_units\":%llu,\"seed_prebuys\":%llu,\"prebought_seeds\":%llu",
                    (unsigned long long)slot_stats.sale_advances, (unsigned long long)slot_stats.advanced_units,
                    (unsigned long long)slot_stats.seed_prebuys, (unsigned long long)slot_stats.seed_units);
                auto array = [](const char* name, const auto* values) {
                    std::printf(",\"%s\":[", name);
                    for (int i = 0; i < kag::N_ITEMS; ++i) std::printf("%s%d", i ? "," : "", int(values[i]));
                    std::printf("]");
                };
                array("own_produced", a.produced); array("rival_produced", b.produced);
                array("own_discarded", a.discarded); array("rival_discarded", b.discarded);
                array("own_final_stock", a.shed); array("rival_final_stock", b.shed);
                std::printf(",\"seconds\":%.6f}\n", std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count());
            }
        }
    }
}
