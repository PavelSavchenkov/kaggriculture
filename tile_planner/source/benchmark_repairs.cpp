#include "replay_trace.hpp"
#include "actions.hpp"
#include "repairs.hpp"
#include <chrono>

struct Fault { std::string kind; int step, cell; double money; };
struct Shops { int count; std::array<uint8_t, 8> types{}; };

static bool funded(const Sim& sim, const Action& action, int seat) {
    const auto accepted = sim.sanitize_solo_action(seat, action);
    for (int s = 0; s < action.n_orders; ++s) {
        const auto& a = action.orders[s]; const auto& b = accepted.orders[s];
        if (a.op == M_NONE || a.op == M_SELL) continue;
        if (a.op != b.op || (a.op != M_HIRE && a.op != M_BUY_LAND && a.n != b.n)) return false;
    }
    return true;
}

int main(int argc, char** argv) {
    try {
        if (argc != 6) throw std::runtime_error("usage: benchmark_repairs trace seat source_course new_output faults_per_kind");
        const auto began = std::chrono::steady_clock::now();
        const auto trace = load(argv[1]); validate(trace);
        const int seat = std::stoi(argv[2]), limit = std::stoi(argv[5]);
        if (seat < 0 || seat > 1 || limit < 1) throw std::runtime_error("invalid argument");
        const fs::path course(argv[3]), output(argv[4]);
        if (fs::exists(output)) throw std::runtime_error("output exists");
        fs::create_directories(output);
        std::vector<Action> plan;
        for (int d = 0; d < 30; ++d) {
            const auto name = (d < 10 ? "0" : "") + std::to_string(d);
            const auto day = labor::offline::read_actions((course / name / "executable.actions.txt").string());
            for (int h = 0; h < 24 && plan.size() < 719; ++h) plan.push_back(day[h]);
        }
        std::vector<Fault> weeds, cash;
        std::array<Shops, 720> shops;
        std::array<bool, 100> empty_at_dawn{};
        std::array<bool, 30> cash_day{};
        std::array<std::array<bool, 100>, 30> weed_day{};
        Sim source(trace.config);
        for (int step = 0; step < 719; ++step) {
            shops[step].count = source.st.n_shops; std::copy_n(source.st.shops, source.st.n_shops, shops[step].types.begin());
            const auto& farm = source.st.farms[seat];
            if (step % 24 == 0) for (int cell = 0; cell < 100; ++cell) empty_at_dawn[cell] = farm.tiles[cell / 10][cell % 10].kind == T_EMPTY;
            if (source.st.day > 0) for (int u = 0; u < farm.n_units; ++u) {
                const auto op = plan[step].units[u].op;
                const int cell = farm.pos_y[u] * 10 + farm.pos_x[u];
                if ((op == OP_PLANT || op == OP_BUILD_COOP || op == OP_BUILD_PASTURE) && empty_at_dawn[cell] && !weed_day[source.st.day][cell]) {
                    weeds.push_back({"weed", source.st.day * 24, cell, -1}); weed_day[source.st.day][cell] = true;
                }
            }
            if (source.st.day > 0 && !cash_day[source.st.day]) {
                bool buys = false;
                for (int s = 0; s < plan[step].n_orders; ++s) buys |= plan[step].orders[s].op != M_NONE && plan[step].orders[s].op != M_SELL;
                if (buys && funded(source, plan[step], seat)) {
                    int low = 0, high = int(farm.money);
                    while (low < high) {
                        const int mid = low + (high - low) / 2;
                        auto probe = source; probe.st.farms[seat].money = mid;
                        if (funded(probe, plan[step], seat)) high = mid; else low = mid + 1;
                    }
                    if (low >= 2) { cash.push_back({"cash", step, -1, double(low - 2)}); cash_day[source.st.day] = true; }
                }
            }
            source.step(trace.turns[step].a[0], trace.turns[step].a[1]);
        }
        shops[719].count = source.st.n_shops; std::copy_n(source.st.shops, source.st.n_shops, shops[719].types.begin());
        auto select = [limit](const std::vector<Fault>& choices) {
            std::vector<Fault> result;
            for (int i = 0; i < std::min(limit, int(choices.size())); ++i) result.push_back(choices[size_t(i) * choices.size() / std::min(limit, int(choices.size()))]);
            return result;
        };
        std::vector<Fault> faults{{"clean", -1, -1, -1}};
        for (const auto& fault : select(weeds)) faults.push_back(fault);
        for (const auto& fault : select(cash)) faults.push_back(fault);
        std::ofstream cases(output / "FAULTS.csv"), report(output / "results.csv");
        cases << "fault,kind,step,cell,money\n";
        for (size_t i = 0; i < faults.size(); ++i) cases << i << ',' << faults[i].kind << ',' << faults[i].step << ',' << faults[i].cell << ',' << faults[i].money << '\n';
        report << "fault,kind,world,mode,money,shock_loss,failed_units,failed_orders,omitted_commands,digs,absorbed_passes,skipped_digs,dropped_commands,cash_attempts,advanced_units,canceled_units,production_exact,inventory_exact,tiles_exact,guard_checks,guard_rejections,proactive_digs";
        for (int item = 0; item < N_PRODUCTS; ++item) report << ",produced_" << item;
        report << '\n';
        for (size_t f = 0; f < faults.size(); ++f) for (int fixed = 0; fixed < 2; ++fixed) for (int mode = 0; mode < 8; ++mode) {
            Sim sim(trace.config); placement::PlanRepair repair;
            int failed_units = 0, failed_orders = 0, omitted = 0; double shock_loss = 0;
            for (int step = 0; step < 719; ++step) {
                if (fixed) { sim.st.n_shops = shops[step].count; std::copy_n(shops[step].types.begin(), shops[step].count, sim.st.shops); }
                auto& farm = sim.st.farms[seat];
                const auto& fault = faults[f];
                if (step == fault.step) {
                    if (fault.kind == "weed") {
                        if (farm.tiles[fault.cell / 10][fault.cell % 10].kind != T_EMPTY) throw std::runtime_error("weed injection target is not empty");
                        farm.tiles[fault.cell / 10][fault.cell % 10].kind = T_WEED;
                        farm.empty_mask[fault.cell >> 6] &= ~(uint64_t{1} << (fault.cell & 63));
                    } else { shock_loss = std::max(0., farm.money - fault.money); farm.money -= shock_loss; }
                }
                auto action = repair.act(farm, sim.st.market, sim.cfg, plan, step, mode);
                for (int u = farm.n_units; u < plan[step].n_units; ++u) omitted += plan[step].units[u].op != OP_PASS;
                Action pair[2] = {trace.turns[step].a[0], trace.turns[step].a[1]}; pair[seat] = action;
                const auto outcome = sim.diagnose_joint_actions(pair[0], pair[1]).players[seat];
                failed_units += outcome.requested_unit_actions - outcome.successful_unit_actions;
                failed_orders += outcome.requested_order_units - outcome.successful_order_units;
                sim.step(pair[0], pair[1]);
            }
            repair.finish_day();
            const auto& stats = repair.stats; const auto& farm = sim.st.farms[seat];
            const auto& expected = source.st.farms[seat];
            bool production_exact = true, inventory_exact = true, tiles_exact = true;
            for (int item = 0; item < N_ITEMS; ++item) {
                production_exact &= farm.produced[item] == expected.produced[item];
                int a = farm.shed[item], b = expected.shed[item];
                for (int u = 0; u < farm.n_units; ++u) a += farm.inv[u][item];
                for (int u = 0; u < expected.n_units; ++u) b += expected.inv[u][item];
                inventory_exact &= a == b;
            }
            for (int item = 0; item < N_CROPS; ++item) inventory_exact &= farm.seeds[item] == expected.seeds[item];
            for (int cell = 0; cell < 100; ++cell) {
                const auto a = managed(farm.tiles[cell / 10][cell % 10], sim.st.day);
                const auto b = managed(expected.tiles[cell / 10][cell % 10], source.st.day);
                const auto idle = [](const auto& tile) { return tile.kind == day_solver::ManagedTileKind::EMPTY || tile.kind == day_solver::ManagedTileKind::WEED; };
                tiles_exact &= a == b || (idle(a) && idle(b));
            }
            if (f == 0 && (farm.money != trace.truth.back().money[seat] || failed_units || failed_orders || omitted || stats.digs || stats.advanced_units))
                throw std::runtime_error("clean control changed or failed: mode " + std::to_string(mode) + " units " + std::to_string(failed_units) +
                    " orders " + std::to_string(failed_orders) + " proactive " + std::to_string(stats.proactive_digs));
            report << f << ',' << faults[f].kind << ',' << (fixed ? "fixed_shops" : "native") << ',' << mode << ',' << farm.money << ',' << shock_loss
                   << ',' << failed_units << ',' << failed_orders << ',' << omitted << ',' << stats.digs << ',' << stats.absorbed_passes << ',' << stats.skipped_digs
                   << ',' << stats.dropped_commands << ',' << stats.cash_attempts << ',' << stats.advanced_units << ',' << stats.canceled_units
                   << ',' << production_exact << ',' << inventory_exact << ',' << tiles_exact << ',' << stats.guard_checks << ',' << stats.guard_rejections << ',' << stats.proactive_digs;
            for (int item = 0; item < N_PRODUCTS; ++item) report << ',' << farm.produced[item];
            report << '\n'; report.flush();
        }
        const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
        std::ofstream summary(output / "SUMMARY.json");
        summary << "{\"faults\":" << faults.size() - 1 << ",\"runs\":" << faults.size() * 16 << ",\"seconds\":" << seconds << ",\"clean_controls_unchanged\":true}\n";
        std::cout << faults.size() * 16 << " runs, seconds " << seconds << '\n';
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
