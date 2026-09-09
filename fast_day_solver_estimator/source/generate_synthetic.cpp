#include "extract_contract.hpp"
#include <random>
#include <stdexcept>

using namespace kag;
using namespace day_solver;
namespace fs = std::filesystem;

struct LocalContract {
    ManagedTileState start, end;
    std::vector<TileWorkAction> work;
    std::array<int64_t, N_ITEMS> input{}, output{};
    std::array<int64_t, N_CROPS> seeds{}, end_seeds{};
};

void check_masks(const Farm& farm) {
    for (int i = 0; i < 100; ++i) {
        const auto& tile = farm.tiles[i / 10][i % 10];
        const uint64_t bit = uint64_t{1} << (i % 64);
        if (bool(farm.empty_mask[i / 64] & bit) != (tile.kind == T_EMPTY) ||
            bool(farm.plant_mask[i / 64] & bit) != (tile.kind == T_PLANT) ||
            bool(farm.animal_mask[i / 64] & bit) != tile.has_animal)
            throw std::runtime_error("synthetic endpoint engine mask mismatch");
    }
}

LocalContract local_contract(int profile, std::mt19937& random) {
    Config config; config.weed_chance = 0; config.starting_money = 1000000; config.shed_capacity = 30000;
    Sim sim(config); auto& farm = sim.st.farms[0]; auto& tile = farm.tiles[4][4];
    std::vector<UnitAction> actions;
    LocalContract result;
    auto act = [&](int op, int arg = 0) { actions.push_back({uint8_t(op), uint8_t(arg), 1}); };
    auto input = [&](int item) { ++result.input[item]; farm.inv_add(0, item, 1); };
    auto plant = [&](int crop) { ++result.seeds[crop]; ++farm.seeds[crop]; act(OP_PLANT, crop); act(OP_WATER); };
    if (profile < 10) {
        const int crop = profile % 5; const auto& definition = CROPS[crop];
        const bool harvest = profile >= 5;
        const int age = harvest ? definition.first_yield_day + int(random() % (definition.ongoing ? 3 : definition.max_yield_day - definition.first_yield_day + 1))
                                : int(random() % definition.first_yield_day);
        tile.kind = T_PLANT; tile.what = crop; tile.planted_day = -age;
        const int possible = definition.ongoing ? (harvest ? std::min(definition.max_yield, 2 * (1 + (age - definition.first_yield_day) / definition.interval)) : 0)
            : std::min(definition.max_yield, 1 + 2 * std::max(0, age - (definition.max_yield_day + 1) / 2));
        tile.yield_units = possible ? 1 + random() % possible : 0;
        tile.max_lifespan_step = definition.ongoing ? -1 : (definition.max_yield_day - age + 1) * 24;
        if (random() % 3 == 0) { input(FERTILIZER); act(OP_FERTILIZE); }
        act(OP_WATER);
        if (harvest) {
            act(OP_HARVEST);
            if (!definition.ongoing && random() % 2) plant(crop);
        }
    } else if (profile < 13) {
        const int animal = GOOSE + profile - 10; const auto& definition = ANIMALS[animal - GOOSE];
        tile.kind = animal == GOOSE ? T_COOP : T_PASTURE; tile.has_animal = true; tile.what = animal;
        tile.planted_day = -(definition.first_yield_day + random() % (definition.interval + 1));
        tile.yield_units = 1 + random() % definition.max_held; tile.fertilizer_available = true;
        tile.pending_care_bonus = random() % (definition.interval + 1);
        act(OP_HARVEST); input(WHEAT); act(OP_FEED); act(OP_CARE); act(OP_COLLECT_FERTILIZER);
    } else if (profile < 16) {
        const int animal = GOOSE + profile - 13;
        act(animal == GOOSE ? OP_BUILD_COOP : OP_BUILD_PASTURE); input(animal); act(OP_PLACE, animal);
        input(WHEAT); act(OP_FEED); act(OP_CARE);
    } else {
        throw std::runtime_error("unknown local profile");
    }
    // Only this local endpoint oracle starts a worker on the tile with inputs.
    // It supplies no route or workforce certificate to the global problem.
    const int cell = 44; const uint64_t bit = uint64_t{1} << cell;
    farm.empty_mask[0] &= ~bit; farm.plant_mask[0] &= ~bit; farm.animal_mask[0] &= ~bit;
    if (tile.kind == T_EMPTY) farm.empty_mask[0] |= bit;
    if (tile.kind == T_PLANT) farm.plant_mask[0] |= bit;
    if (tile.has_animal) farm.animal_mask[0] |= bit;
    if (tile.max_lifespan_step >= 0) { farm.decay_mask[0] |= bit; farm.next_decay_step = tile.max_lifespan_step; }
    result.start = labor::offline::managed(tile, 0);
    check_masks(farm);
    for (int hour = 0; hour < 24; ++hour) {
        Action own, other;
        if (hour < int(actions.size())) own.units[0] = actions[hour];
        own.finalize(); other.finalize();
        const auto accepted = sim.sanitize_solo_action(0, own);
        if (accepted.units[0].op != own.units[0].op) throw std::runtime_error("local synthetic task rejected");
        const auto before = sim.st.farms[0];
        sim.step(own, other); check_masks(sim.st.farms[0]);
        if (hour >= int(actions.size())) continue;
        const auto action = actions[hour]; TileWorkAction work; work.op = action.op;
        if (action.op == OP_PLANT || action.op == OP_PLACE) work.arg = action.arg;
        if (action.op == OP_COLLECT_FERTILIZER) { work.output_item = FERTILIZER; work.output_quantity = 1; }
        if (action.op == OP_HARVEST) {
            for (int item = 0; item < N_PRODUCTS; ++item) {
                const int n = sim.st.farms[0].produced[item] - before.produced[item];
                if (n) { work.arg = work.output_item = item; work.output_quantity = n; }
            }
            if (!work.output_quantity) throw std::runtime_error("synthetic empty harvest");
        }
        result.work.push_back(work);
    }
    result.end = labor::offline::managed(sim.st.farms[0].tiles[4][4], 1);
    std::copy_n(sim.st.farms[0].shed, N_ITEMS, result.output.begin());
    std::copy_n(sim.st.farms[0].seeds, N_CROPS, result.end_seeds.begin());
    return result;
}

DayProblem make_problem(int profile, int count, int geometry, int deadline, int release, uint32_t seed) {
    std::mt19937 random(seed);
    std::vector<std::pair<int, int>> cells;
    for (int y = 0; y < 10; ++y) for (int x = 0; x < 10; ++x) cells.emplace_back(x, y);
    auto distance = [](const auto& cell) { return std::min(std::abs(cell.first - 4), std::abs(cell.first - 5))
        + std::min(std::abs(cell.second - 4), std::abs(cell.second - 5)); };
    if (geometry == 2) std::shuffle(cells.begin(), cells.end(), random);
    else std::stable_sort(cells.begin(), cells.end(), [&](const auto& a, const auto& b) {
        return geometry == 0 ? distance(a) < distance(b) : distance(a) > distance(b); });
    DayProblem problem;
    for (int i = 0; i < count; ++i) {
        auto local = local_contract(profile == 16 ? random() % 16 : profile, random);
        problem.start.managed_tiles.push_back({int8_t(cells[i].first), int8_t(cells[i].second), local.start});
        EndTileRequirement end; end.tile = i; end.exact_state = local.end; problem.required_end_tiles.push_back(end);
        problem.tile_work.push_back({int16_t(i), local.work});
        for (int item = 0; item < N_ITEMS; ++item) { problem.start.shed[item] += local.input[item]; problem.end_shed[item] += local.output[item]; }
        for (int crop = 0; crop < N_CROPS; ++crop) { problem.start.seeds[crop] += local.seeds[crop]; problem.end_seeds[crop] += local.end_seeds[crop]; }
    }
    if (release >= 0) {
        int slot = 0;
        for (int item = 0; item < N_ITEMS; ++item) if (problem.start.shed[item]) {
            MarketEvent event; event.hour = release; event.order_index = slot++;
            event.market_op = item >= GOOSE ? M_BUY_ANIMAL : M_BUY_PRODUCT; event.item = item;
            event.quantity = problem.start.shed[item]; problem.start.shed[item] = 0; problem.market_plan.push_back(event);
        }
        // At most five distinct input types occur: wheat, fertilizer and animals.
        if (slot > 10) throw std::runtime_error("too many synthetic input purchases");
    }
    if (deadline >= 0) for (int item = 0; item < N_PRODUCTS; ++item) {
        const auto amount = problem.end_shed[item] * 3 / 4;
        for (int hour = deadline; hour < 24; ++hour) problem.shed_availability[hour][item] = amount;
        problem.end_shed[item] -= amount;
    }
    int tasks = 0; for (const auto& work : problem.tile_work) tasks += work.actions.size();
    // An offline query ceiling, never a workforce label or estimator feature.
    problem.worker_count = std::min(40, 2 + (tasks + 3 * count + 15) / 16);
    for (int worker = 1; worker < problem.worker_count; ++worker) {
        int available = -1;
        for (int slot = 0; slot < 230 && available < 0; ++slot) {
            const bool occupied = std::any_of(problem.market_plan.begin(), problem.market_plan.end(), [&](const auto& e) {
                return e.hour == slot / 10 && e.order_index == slot % 10; });
            if (!occupied) available = slot;
        }
        MarketEvent event; event.hour = available / 10; event.order_index = available % 10;
        event.market_op = M_HIRE; event.item = -1; event.quantity = 1; problem.market_plan.push_back(event);
    }
    day_scheduler::prepare_problem(problem);
    return problem;
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: generate_synthetic manifest.txt output_dir");
        std::ifstream input(argv[1]); const fs::path output(argv[2]);
        if (!input || fs::exists(output)) throw std::runtime_error("invalid input/output exists");
        fs::create_directories(output);
        std::string id; uint32_t seed; int profile, count, geometry, deadline, release, completed = 0;
        while (input >> id >> seed >> profile >> count >> geometry >> deadline >> release) {
            if (count < 1 || count > 96 || profile < 0 || profile > 16 || geometry < 0 || geometry > 2 ||
                deadline < -1 || deadline > 23 || release < -1 || release > 23) throw std::runtime_error("bad synthetic parameters");
            save_problem_json(make_problem(profile, count, geometry, deadline, release, seed), output / (id + ".json"));
            ++completed;
        }
        if (!input.eof()) throw std::runtime_error("bad manifest");
        std::cout << "Generated " << completed << " contracts from independently simulated local tile endpoints; no global schedule certificates.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
