#include "seed_suffix_bounds.hpp"
#include "release_bounds.hpp"
#include "extract_contract.hpp"
#include <chrono>
#include <iomanip>
#include <random>

using namespace kag;
using namespace day_solver;
namespace fs = std::filesystem;

DayProblem construct(int count, int hours, int profile, uint32_t seed, const fs::path& directory) {
    Config config; config.weed_chance = 0; config.starting_money = 1000000000;
    Sim sim(config);
    std::fill_n(sim.st.farms[0].seeds, N_CROPS, 0);
    labor::offline::RecordedDay day(sim);
    auto& p = day.problem; p.worker_count = 2 * count;
    std::vector<int> cells;
    for (int y = 0; y < 5; ++y) for (int x = 0; x < 5; ++x) cells.push_back(y * 10 + x);
    std::mt19937 random(seed); std::shuffle(cells.begin(), cells.end(), random);
    std::array<int, N_CROPS> needed{};
    for (int i = 0; i < count; ++i) {
        const int crop = profile == N_CROPS ? i % N_CROPS : profile;
        TileWorkAction plant, water; plant.op = OP_PLANT; plant.arg = crop; water.op = OP_WATER;
        p.tile_work.push_back({int16_t(cells[i]), {plant, water}}); ++needed[crop];
    }
    int slot = 0;
    for (int crop = 0; crop < N_CROPS; ++crop) if (needed[crop])
        p.market_plan.push_back({int8_t(hours - 2), int8_t(slot++), M_BUY_SEED, int16_t(crop), needed[crop], 0});
    const auto menu = labor::earliest_menu(p, hours);
    for (int worker = 1; worker < p.worker_count; ++worker)
        p.market_plan.push_back({int8_t(menu.hours[worker - 1]), int8_t(menu.slots[worker - 1]), M_HIRE, -1, 1, 0});
    std::array<Action, 24> schedule;
    for (int h = 0; h < 24; ++h) {
        auto& farm = sim.st.farms[0]; Action own, rival; own.n_units = farm.n_units;
        for (const auto& e : p.market_plan) if (e.hour == h) {
            own.n_orders = std::max(own.n_orders, int(e.order_index) + 1);
            own.orders[e.order_index] = {e.market_op, uint8_t(std::max(0, int(e.item))), e.quantity};
        }
        for (int u = 0; u < farm.n_units; ++u) {
            const int target = cells[u / 2], x = target % 10, y = target / 10;
            if (h == hours - 1) {
                if (farm.pos_x[u] != x || farm.pos_y[u] != y) throw std::runtime_error("construction missed prepositioning");
                const int crop = profile == N_CROPS ? (u / 2) % N_CROPS : profile;
                own.units[u] = {uint8_t(u % 2 ? OP_WATER : OP_PLANT), uint8_t(u % 2 ? 0 : crop), 1};
            } else if (h < hours - 1) {
                if (farm.pos_x[u] > x) own.units[u].op = OP_WEST;
                else if (farm.pos_x[u] < x) own.units[u].op = OP_EAST;
                else if (farm.pos_y[u] > y) own.units[u].op = OP_NORTH;
                else if (farm.pos_y[u] < y) own.units[u].op = OP_SOUTH;
            }
        }
        own.finalize(); rival.finalize();
        const auto accepted = sim.sanitize_solo_action(0, own);
        for (int u = 0; u < own.n_units; ++u)
            if (accepted.units[u].op != own.units[u].op) throw std::runtime_error("construction action sanitized");
        schedule[h] = own; sim.step(own, rival);
    }
    const auto& farm = sim.st.farms[0];
    std::copy_n(farm.shed, N_ITEMS, p.end_shed.begin()); std::copy_n(farm.seeds, N_CROPS, p.end_seeds.begin());
    for (int cell = 0; cell < 100; ++cell) {
        EndTileRequirement end; end.tile = cell; end.exact_state = labor::offline::managed(farm.tiles[cell / 10][cell % 10], 1);
        p.required_end_tiles.push_back(end);
    }
    for (int i = 0; i < count; ++i)
        if (farm.tiles[cells[i] / 10][cells[i] % 10].kind != T_PLANT)
            throw std::runtime_error("planted crop did not survive verified watering");
    day_scheduler::prepare_problem(p);
    const auto replay = replay_schedule(p, schedule);
    if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())
        throw std::runtime_error("constructed schedule fails strict physical replay");
    const auto bound = labor::seed_suffix_bound(p, menu, hours);
    if (bound.workers != 2 * count) throw std::runtime_error("seed suffix bound misses constructed optimum");
    const std::string id = "h" + std::to_string(hours) + "_p" + std::to_string(profile) + "_n" + std::to_string(count);
    save_problem_json(p, directory / (id + ".json")); labor::offline::save_actions(schedule, directory / (id + ".actions"));
    return p;
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: check_seed_suffix new_output_directory");
        const fs::path output(argv[1]);
        if (fs::exists(output)) throw std::runtime_error("output already exists");
        fs::create_directories(output / "contracts");
        std::ofstream results(output / "EXACT.csv");
        results << "id,active_hours,plants,profile,verified_workers,old_release_bound,seed_suffix_bound,cpu_us\n";
        int controls = 0;
        for (int hours : {23, 24}) for (int profile = 0; profile <= N_CROPS; ++profile) for (int n = 1; n <= 20; ++n) {
            auto p = construct(n, hours, profile, 9091327 + 1009 * profile + 31 * n + hours, output / "contracts");
            const auto menu = labor::earliest_menu(p, hours);
            const auto old = labor::release_bound(p, menu, hours);
            const auto before = std::chrono::steady_clock::now();
            labor::SeedSuffixBound bound;
            for (int repeat = 0; repeat < 100; ++repeat) bound = labor::seed_suffix_bound(p, menu, hours);
            const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - before).count() / 100;
            results << "h" << hours << "_p" << profile << "_n" << n << ',' << hours << ',' << n << ',' << profile << ','
                    << p.worker_count << ',' << old.workers << ',' << bound.workers << ',' << std::setprecision(9) << us << '\n';
            auto changed = p; changed.worker_count = 1;
            std::erase_if(changed.market_plan, [](const auto& e) { return e.market_op == M_HIRE; });
            if (labor::seed_suffix_bound(changed, menu, hours) != bound) throw std::runtime_error("seed suffix answer leakage");
            std::reverse(changed.tile_work.begin(), changed.tile_work.end());
            if (labor::seed_suffix_bound(changed, menu, hours) != bound) throw std::runtime_error("seed suffix order dependence");
            for (int crop = 0; crop < N_CROPS; ++crop) changed.start.seeds[crop] = 100;
            if (labor::seed_suffix_bound(changed, menu, hours).workers != 1) throw std::runtime_error("initial seeds ignored");
            for (int crop = 0; crop < N_CROPS; ++crop) changed.start.seeds[crop] = 0;
            std::erase_if(changed.market_plan, [](const auto& e) { return e.market_op == M_BUY_SEED; });
            if (labor::seed_suffix_bound(changed, menu, hours).workers != 41) throw std::runtime_error("missing seeds missed");
            controls += 5;
        }
        std::cout << "Verified 240 exact-workforce contracts and " << controls << " controls.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
