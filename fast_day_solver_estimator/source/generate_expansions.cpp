#include "local_asset.hpp"
#include "bounds.hpp"
#include <day_solver/io.hpp>
#include <fstream>
#include <iostream>
#include <random>
#include <span>

using namespace day_solver;
namespace fs = std::filesystem;

bool insert_buy(DayProblem& p, int hour, int op, int item, int quantity) {
    for (int slot = 0; slot < 10; ++slot) {
        if (std::any_of(p.market_plan.begin(), p.market_plan.end(), [&](const auto& e) { return e.hour == hour && e.order_index == slot; })) continue;
        p.market_plan.push_back({int8_t(hour), int8_t(slot), uint8_t(op), int16_t(item), quantity});
        return true;
    }
    return false;
}

std::string expand(DayProblem& p, int count, int item, int geometry, int release, int land_hour, uint32_t seed) {
    if (!count) return "OK";
    std::erase_if(p.market_plan, [](const auto& e) { return e.market_op == kag::M_HIRE; });
    p.worker_count = 1; // Unscheduled placeholder; never a claimed workforce.
    std::array<bool, 100> worked{};
    for (const auto& w : p.tile_work) if (!w.actions.empty()) worked[w.tile] = true;
    int quadrant = -1;
    if (land_hour >= 0) {
        for (const auto& tile : p.start.managed_tiles) if (tile.state.kind == ManagedTileKind::LOCKED) {
            const int q = (tile.x >= 5) + 2 * (tile.y >= 5);
            if (quadrant < 0 || q < quadrant) quadrant = q;
        }
        if (quadrant < 0) return "NO_UNOWNED_QUADRANT";
        const bool planned = std::any_of(p.market_plan.begin(), p.market_plan.end(), [&](const auto& e) {
            return e.market_op == kag::M_BUY_LAND && e.item == quadrant;
        });
        if (!planned && !insert_buy(p, land_hour, kag::M_BUY_LAND, quadrant, 1)) return "NO_LAND_ORDER_SLOT";
    }
    std::vector<int> available;
    for (int i = 0; i < int(p.start.managed_tiles.size()); ++i) {
        const auto& tile = p.start.managed_tiles[i];
        const bool owned = tile.state.kind == ManagedTileKind::EMPTY || tile.state.kind == ManagedTileKind::WEED;
        const bool land = tile.state.kind == ManagedTileKind::LOCKED && (tile.x >= 5) + 2 * (tile.y >= 5) == quadrant;
        if (!worked[i] && (land_hour >= 0 ? land : owned)) available.push_back(i);
    }
    if (int(available.size()) < count) return "TOO_FEW_UNUSED_TILES";
    std::mt19937 random(seed);
    std::shuffle(available.begin(), available.end(), random);
    if (geometry != 2) std::stable_sort(available.begin(), available.end(), [&](int a, int b) {
        const auto& x = p.start.managed_tiles[a]; const auto& y = p.start.managed_tiles[b];
        const int da = labor::shed_distance(x.y * 10 + x.x), db = labor::shed_distance(y.y * 10 + y.x);
        return geometry == 0 ? da < db : da > db;
    });
    auto end_state = [&](int tile) -> ManagedTileState& {
        auto found = std::find_if(p.required_end_tiles.begin(), p.required_end_tiles.end(), [&](const auto& e) { return e.tile == tile; });
        if (found == p.required_end_tiles.end() || !found->exact_state) throw std::runtime_error("expansion needs explicit original endpoints");
        return *found->exact_state;
    };
    if (quadrant >= 0) for (int i = 0; i < int(p.start.managed_tiles.size()); ++i) {
        const auto& tile = p.start.managed_tiles[i];
        if (tile.state.kind == ManagedTileKind::LOCKED && (tile.x >= 5) + 2 * (tile.y >= 5) == quadrant && end_state(i).kind == ManagedTileKind::LOCKED)
            end_state(i) = {};
    }
    std::array<int, kag::N_ITEMS> inputs{};
    std::array<int, kag::N_CROPS> seeds{};
    for (int tile : std::span(available.data(), count)) {
        auto asset = labor::offline::new_asset(item, p.start.managed_tiles[tile].state.kind == ManagedTileKind::WEED);
        p.tile_work.push_back({int16_t(tile), asset.work}); end_state(tile) = asset.end;
        for (int i = 0; i < kag::N_ITEMS; ++i) inputs[i] += asset.inputs[i];
        for (int i = 0; i < kag::N_CROPS; ++i) seeds[i] += asset.seeds[i];
    }
    for (int i = 0; i < kag::N_ITEMS; ++i) if (inputs[i] && !insert_buy(p, release, i >= kag::GOOSE ? kag::M_BUY_ANIMAL : kag::M_BUY_PRODUCT, i, inputs[i]))
        return "NO_INPUT_ORDER_SLOT";
    for (int i = 0; i < kag::N_CROPS; ++i) if (seeds[i] && !insert_buy(p, release, kag::M_BUY_SEED, i, seeds[i])) return "NO_SEED_ORDER_SLOT";
    std::sort(p.market_plan.begin(), p.market_plan.end(), [](const auto& a, const auto& b) { return std::pair(a.hour, a.order_index) < std::pair(b.hour, b.order_index); });
    day_scheduler::prepare_problem(p);
    return "OK";
}

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: generate_expansions manifest output_directory");
        std::ifstream input(argv[1]); const fs::path output = argv[2];
        if (!input || fs::exists(output)) throw std::runtime_error("invalid input/output exists");
        fs::create_directories(output); std::ofstream records(output / "STATUS.csv");
        records << "id,status\n";
        std::string id, path; int count, item, geometry, release, land; uint32_t seed;
        while (input >> id >> path >> count >> item >> geometry >> release >> land >> seed) {
            if (count < 0 || count > 25 || geometry < 0 || geometry > 2 || release < 0 || release > 22 || land < -1 || land > 22)
                throw std::runtime_error("invalid expansion parameters");
            auto p = load_problem_json(path);
            const auto status = expand(p, count, item, geometry, release, land, seed);
            if (status == "OK") save_problem_json(p, output / (id + ".json"));
            records << id << ',' << status << '\n';
        }
        if (!input.eof()) throw std::runtime_error("malformed expansion manifest");
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
