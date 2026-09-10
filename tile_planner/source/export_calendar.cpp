#include "course.hpp"
#include "finance_snapshot/compile_calendar.hpp"
#include "shop_scenario.hpp"
#include "recorded_actions.hpp"
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace placement;
    namespace sp = sales_planner;
    try {
        if (argc != 6) throw std::runtime_error("usage: export_calendar compiled_course seed weed_chance seat new_output");
        const fs::path source(argv[1]), output(argv[5]); const int seat = std::stoi(argv[4]);
        if (seat < 0 || seat > 1 || fs::exists(output)) throw std::runtime_error("invalid seat or output exists");
        fs::create_directories(output);
        kag::Config config; config.seed = std::stoull(argv[2]); config.weed_chance = std::stod(argv[3]); kag::Sim sim(config);
        auto financial = sp::financial_state(sim.st);
        std::ofstream events(output / "events.csv"), orders(output / "orders.csv"), states(output / "states.csv");
        std::ofstream deliveries(output / "deliveries.csv"); deliveries << "turn,phase,item,delivered,discarded\n";
        std::ifstream shops; if (fs::exists(source / "shops.txt")) shops.open(source / "shops.txt");
        events << "turn,phase,sequence,flow,item,buffer,quantity\n";
        orders << "turn,slot,op,item,quantity,commitment\n";
        states << "turn,cash,shed_total,workers,quadrants\n" << std::setprecision(17);
        int projections = 0, event_count = 0, rival_invalid_item_noops = 0;
        const auto began = std::chrono::steady_clock::now();
        for (int d = 0; d < 30; ++d) {
            const auto folder = source / day_name(d);
            const auto own = labor::offline::read_actions((folder / "executable.actions.txt").string());
            Schedule rival;
            for (auto& action : rival) { action.n_units = 1; action.finalize(); }
            if (fs::exists(folder / "opponent.actions.txt")) rival = read_recorded_actions(folder / "opponent.actions.txt");
            for (int h = 0; h < (d == 29 ? 23 : 24); ++h) {
                const int t = d * 24 + h;
                if (shops.is_open()) load_shops(shops, sim, t);
                const auto obs = kag::agent::runtime::make_observation(sim, seat);
                sp::CalendarTurn turn;
                if (!sp::worker_calendar(obs, own[h], turn, config.shed_capacity, 24, projections)) throw std::runtime_error("worker calendar projection mismatch");
                int phase = 0;
                for (const auto* effects : {&turn.before_market, &turn.after_market}) {
                    int sequence = 0;
                    for (const auto& e : *effects) { events << t << ',' << phase << ',' << sequence++ << ',' << int(e.flow) << ',' << +e.item << ',' << +e.buffer << ',' << e.quantity << '\n'; ++event_count; }
                    ++phase;
                }
                for (int s = 0; s < own[h].n_orders; ++s) {
                    const auto& order = own[h].orders[s];
                    orders << t << ',' << s << ',' << +order.op << ',' << +order.item << ',' << order.n << ','
                           << (order.op == kag::M_HIRE || order.op == kag::M_BUY_LAND) << '\n';
                }
                // This verifier may inspect the whole offline game. The
                // exported own worker calendar above reads only obs.self().
                financial = sp::financial_state(sim.st);
                std::array<sp::Resources, 2> buffers;
                buffers[0] = sp::own_resources(kag::agent::runtime::make_observation(sim, 0));
                buffers[1] = sp::own_resources(kag::agent::runtime::make_observation(sim, 1));
                sp::CalendarTurn other;
                auto projected_rival = rival[h];
                // The recorded opponent may emit an item-less command. The
                // engine ignores it, while the own-calendar adapter requires
                // valid item indices. Normalize only these proven no-ops for
                // the ledger; replay the original action in the full engine.
                for (int u = 0; u < projected_rival.n_units; ++u) {
                    auto& action = projected_rival.units[u];
                    if (((action.op == kag::OP_PICKUP || action.op == kag::OP_PLACE) && action.arg >= kag::N_ITEMS) ||
                        (action.op == kag::OP_PLANT && action.arg >= kag::N_CROPS)) {
                        action = {}; ++rival_invalid_item_noops;
                    }
                }
                projected_rival.finalize();
                if (!sp::worker_calendar(kag::agent::runtime::make_observation(sim, seat ^ 1), projected_rival, other, config.shed_capacity, 24, projections))
                    throw std::runtime_error("opponent calendar projection mismatch");
                auto apply_own = [&](const sp::CalendarTurn& calendar, bool after_market) {
                    for (const auto& event : after_market ? calendar.after_market : calendar.before_market) {
                        const auto stock = financial.accounts[seat].stock, discarded = buffers[seat].discarded;
                        sp::apply(financial.accounts[seat], buffers[seat], std::span(&event, 1));
                        for (int item = 0; item < kag::N_ITEMS; ++item) {
                            const int delivered = financial.accounts[seat].stock[item] - stock[item];
                            const int dropped = buffers[seat].discarded[item] - discarded[item];
                            if (delivered > 0 || dropped > 0) deliveries << t << ',' << after_market << ',' << item << ',' << delivered << ',' << dropped << '\n';
                        }
                    }
                };
                apply_own(turn, false);
                sp::apply(financial.accounts[seat ^ 1], buffers[seat ^ 1], other.before_market);
                std::array<sp::Orders, 2> pair;
                pair[seat].count = own[h].n_orders; std::copy_n(own[h].orders, own[h].n_orders, pair[seat].values.begin());
                pair[seat ^ 1].count = rival[h].n_orders; std::copy_n(rival[h].orders, rival[h].n_orders, pair[seat ^ 1].values.begin());
                sp::trade(financial, pair); sp::consume(financial);
                apply_own(turn, true);
                sp::apply(financial.accounts[seat ^ 1], buffers[seat ^ 1], other.after_market); sp::advance(financial);
                kag::Action actions[2]; actions[seat] = own[h]; actions[seat ^ 1] = rival[h];
                const auto diagnosis = sim.diagnose_joint_actions(actions[0], actions[1]).players[seat];
                if (diagnosis.requested_unit_actions != diagnosis.successful_unit_actions || diagnosis.requested_order_units != diagnosis.successful_order_units)
                    throw std::runtime_error("compiled course failed in calendar replay");
                sim.step(actions[0], actions[1]);
                const auto after = kag::agent::runtime::make_observation(sim, seat);
                if (!sp::same_resources(financial.accounts[seat], buffers[seat], sp::own_account(after), sp::own_resources(after)) ||
                    financial.accounts[seat].cash != after.self().money || !std::equal(financial.inventory.begin(), financial.inventory.end(), sim.st.market.inventory))
                    throw std::runtime_error("financial calendar and full engine disagree");
                states << t + 1 << ',' << after.self().money << ',' << after.own.shed_total << ',' << after.self().n_units << ',' << after.self().n_quadrants << '\n';
            }
        }
        std::ofstream metadata(output / "CALENDAR.json");
        metadata << "{\"schema\":\"placement_resource_calendar_v1\",\"start\":0,\"end_exclusive\":719,\"schedule_certified\":true,\"verified_transitions\":719,"
                 << "\"events\":" << event_count << ",\"projections\":" << projections << ",\"cash\":" << sim.st.farms[seat].money
                 << ",\"rival_invalid_item_noops\":" << rival_invalid_item_noops
                 << ",\"seconds\":" << std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count()
                 << ",\"phase\":{\"0\":\"before_market\",\"1\":\"after_market_day_end\"},\"flow\":{\"0\":\"produce\",\"1\":\"use\",\"2\":\"withdraw\",\"3\":\"deposit\",\"4\":\"drop\",\"5\":\"use_seed\",\"6\":\"drop_all\"},"
                 << "\"starting_account\":{\"cash\":3000,\"stock\":[],\"seeds\":[],\"buffers\":[],\"workers\":1,\"quadrants\":1},\"capacity\":100,\"max_orders\":10,\"certification_scope\":\"this complete realized course; future reuse requires verification\"}\n";
        std::cout << "719 financial/engine transitions agree; " << event_count << " resource events\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
