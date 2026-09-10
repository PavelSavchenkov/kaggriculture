#include "replay_trace.hpp"
#include "certify.hpp"
#include "timeline.hpp"

int main(int argc, char** argv) {
    using namespace placement;
    try {
        if (argc != 7) throw std::runtime_error("usage: replay_course trace seat course_certificate layout output_json native|fixed_shops");
        const auto trace = load(argv[1]); validate(trace);
        const int seat = std::stoi(argv[2]);
        if (seat < 0 || seat > 1) throw std::runtime_error("invalid seat");
        const fs::path folder(argv[3]);
        const auto timeline = read_timeline(argv[4]);
        const std::string mode(argv[6]);
        if (mode != "native" && mode != "fixed_shops") throw std::runtime_error("invalid world mode");
        std::array<Schedule, 30> schedules;
        for (int d = 0; d < 30; ++d) schedules[d] = labor::offline::read_actions((folder / day_name(d) / "executable.actions.txt").string());
        Sim source(trace.config), candidate(trace.config);
        std::string failure;
        int failed_step = -1, verified_steps = 0, shop_differences = 0;
        int64_t source_bill = 0, candidate_bill = 0;
        auto reject = [&](int step, const std::string& reason) { if (failure.empty()) { failure = reason; failed_step = step; } };
        for (int step = 0; step < 719; ++step) {
            const auto& layout = timeline[step / 24];
            std::array<bool, 100> seen{};
            for (auto cell : layout) { if (cell >= 100 || seen[cell]) throw std::runtime_error("non-bijective timeline"); seen[cell] = true; }
            if (step % 24 == 0 && step > 0) {
                const auto& previous = timeline[step / 24 - 1]; std::array<int, 100> inverse{};
                for (int i = 0; i < 100; ++i) inverse[previous[i]] = i;
                const auto& farm = source.st.farms[seat];
                for (int i = 0; i < 100; ++i) if (layout[i] != previous[i]) {
                    const auto state = managed(farm.tiles[i / 10][i % 10], source.st.day);
                    const int other = inverse[layout[i]];
                    if (!reassignable(state) || state != managed(farm.tiles[other / 10][other % 10], source.st.day) ||
                        (state.kind == day_solver::ManagedTileKind::LOCKED && quadrant(layout[i]) != quadrant(previous[i])))
                        throw std::runtime_error("timeline changes farm state at day boundary");
                }
            }
            if (mode == "fixed_shops") {
                candidate.st.n_shops = source.st.n_shops;
                std::copy_n(source.st.shops, source.st.n_shops, candidate.st.shops);
            }
            const auto& raw = trace.turns[step];
            Action pair[2] = {raw.a[0], raw.a[1]};
            pair[seat] = schedules[step / 24][step % 24];
            const auto outcome = candidate.diagnose_joint_actions(pair[0], pair[1]).players[seat];
            if (outcome.successful_unit_actions != outcome.requested_unit_actions)
                reject(step, "failed_unit_action");
            if (outcome.successful_order_units != outcome.requested_order_units)
                reject(step, "failed_market_order");
            if (!failure.empty()) {
                std::cerr << "first failure step " << step << ' ' << failure << " cash " << candidate.st.farms[seat].money << '\n';
                const auto& farm = candidate.st.farms[seat];
                for (int u = 0; u < pair[seat].n_units; ++u) if (pair[seat].units[u].op != kag::OP_PASS)
                    std::cerr << "worker " << u << " at " << +farm.pos_x[u] << ',' << +farm.pos_y[u] << " op " << +pair[seat].units[u].op
                              << " arg " << +pair[seat].units[u].arg << " quantity " << pair[seat].units[u].n << '\n';
                break;
            }
            const int source_hires_before = source.st.farms[seat].hires_today;
            const int candidate_hires_before = candidate.st.farms[seat].hires_today;
            const int source_units_before = source.st.farms[seat].n_units;
            const int candidate_units_before = candidate.st.farms[seat].n_units;
            if (step % 24 == 23) {
                // Hires on the reset phase have no useful work. Source tapes
                // are still accounted through their exact action diagnostics.
                for (int i = 0; i < raw.a[seat].n_orders; ++i) if (raw.a[seat].orders[i].op == kag::M_HIRE)
                    reject(step, "unsupported_source_terminal_hire");
            }
            source.step(raw.a[0], raw.a[1]); candidate.step(pair[0], pair[1]);
            if (step % 24 != 23) {
                for (int i = source_hires_before; i < source_hires_before + source.st.farms[seat].n_units - source_units_before; ++i) source_bill += kag::fib(i);
                for (int i = candidate_hires_before; i < candidate_hires_before + candidate.st.farms[seat].n_units - candidate_units_before; ++i) candidate_bill += kag::fib(i);
            }
            if (source.st.n_shops != candidate.st.n_shops || !std::equal(source.st.shops, source.st.shops + source.st.n_shops, candidate.st.shops)) ++shop_differences;
            const auto& expected = source.st.farms[seat]; const auto& actual = candidate.st.farms[seat];
            if (actual.money < 0 || actual.shed_total > trace.config.shed_capacity) reject(step, "cash_or_capacity_invariant");
            if (step % 24 == 23 || step == 718) {
                for (int item = 0; item < kag::N_ITEMS; ++item) {
                    int64_t a = actual.shed[item], b = expected.shed[item];
                    if (step == 718) {
                        for (int u = 0; u < actual.n_units; ++u) a += actual.inv[u][item];
                        for (int u = 0; u < expected.n_units; ++u) b += expected.inv[u][item];
                    }
                    if (a != b) reject(step, "inventory_endpoint_item_" + std::to_string(item));
                    if (actual.produced[item] != expected.produced[item]) reject(step, "production_item_" + std::to_string(item));
                    if (actual.sold_units[item] != expected.sold_units[item]) reject(step, "sales_item_" + std::to_string(item));
                }
                for (int item = 0; item < kag::N_CROPS; ++item) if (actual.seeds[item] != expected.seeds[item]) reject(step, "seed_endpoint");
                for (int cell = 0; cell < 100; ++cell) {
                    const int target = layout[cell];
                    auto a = managed(actual.tiles[target / 10][target % 10], candidate.st.day);
                    auto b = managed(expected.tiles[cell / 10][cell % 10], source.st.day);
                    const auto idle = [](const auto& state) { return state.kind == day_solver::ManagedTileKind::EMPTY || state.kind == day_solver::ManagedTileKind::WEED; };
                    if (a != b && !(idle(a) && idle(b))) reject(step, "tile_endpoint_cell_" + std::to_string(cell));
                }
            }
            if (!failure.empty()) break;
            ++verified_steps;
        }
        const bool valid = failure.empty() && verified_steps == 719;
        std::ofstream report(argv[5]);
        report << "{\"valid\":" << (valid ? "true" : "false") << ",\"verified_steps\":" << verified_steps << ",\"failed_step\":" << failed_step
               << ",\"failure\":\"" << failure << "\",\"mode\":\"" << mode << "\",\"shop_difference_steps\":" << shop_differences
               << ",\"source_cash\":" << source.st.farms[seat].money << ",\"candidate_cash\":" << candidate.st.farms[seat].money
               << ",\"cash_gain\":" << candidate.st.farms[seat].money - source.st.farms[seat].money
               << ",\"source_hire_bill\":" << source_bill << ",\"candidate_hire_bill\":" << candidate_bill
               << ",\"hire_saving\":" << source_bill - candidate_bill << ",\"opponent_mode\":\"recorded_actions\"}\n";
        std::cout << (valid ? "full game verified" : failure) << "; steps " << verified_steps << '\n';
        return valid ? 0 : 1;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
