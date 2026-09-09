#pragma once
#include "extract_contract.hpp"

namespace labor::offline {
struct NewAsset {
    day_solver::ManagedTileState end;
    std::vector<day_solver::TileWorkAction> work;
    std::array<int, kag::N_ITEMS> inputs{};
    std::array<int, kag::N_CROPS> seeds{};
};

// Defines one new asset's exact local day-end state. The local worker begins
// on the tile with inputs, so this supplies no global route or workforce proof.
inline NewAsset new_asset(int item, bool weed) {
    using namespace kag;
    if (!is_crop(item) && !is_animal(item)) throw std::runtime_error("unsupported new asset");
    Config config; config.weed_chance = 0; config.starting_money = 1000000; config.shed_capacity = 30000;
    Sim sim(config); auto& farm = sim.st.farms[0];
    farm.tiles[4][4] = {}; farm.tiles[4][4].kind = weed ? T_WEED : T_EMPTY;
    constexpr uint64_t bit = uint64_t{1} << 44;
    farm.empty_mask[0] = (farm.empty_mask[0] & ~bit) | (weed ? 0 : bit);
    farm.plant_mask[0] &= ~bit; farm.animal_mask[0] &= ~bit; farm.decay_mask[0] &= ~bit;
    NewAsset result; result.work.reserve(5);
    auto add = [&](int op, int arg = -1) {
        day_solver::TileWorkAction work; work.op = op; work.arg = arg; result.work.push_back(work);
    };
    auto input = [&](int value) { ++result.inputs[value]; farm.inv_add(0, value, 1); };
    if (weed) add(OP_DIG);
    if (is_crop(item)) {
        ++result.seeds[item]; ++farm.seeds[item];
        add(OP_PLANT, item); add(OP_WATER); input(FERTILIZER); add(OP_FERTILIZE);
    } else {
        add(item == GOOSE ? OP_BUILD_COOP : OP_BUILD_PASTURE);
        input(item); add(OP_PLACE, item); input(WHEAT); add(OP_FEED); add(OP_CARE);
    }
    for (int hour = 0; hour < 24; ++hour) {
        Action action, other;
        if (hour < int(result.work.size())) {
            const auto& work = result.work[hour];
            action.units[0] = {work.op, uint8_t(std::max(0, int(work.arg))), work.quantity};
        }
        action.finalize(); other.finalize();
        if (sim.sanitize_solo_action(0, action).units[0].op != action.units[0].op)
            throw std::runtime_error("new-asset endpoint action rejected");
        sim.step(action, other);
    }
    for (int value : farm.shed) if (value) throw std::runtime_error("unexpected new-asset output or leftover input");
    for (int value : farm.seeds) if (value) throw std::runtime_error("unconsumed new seed");
    for (int value : farm.produced) if (value) throw std::runtime_error("unexpected first-day production");
    result.end = managed(farm.tiles[4][4], 1);
    return result;
}
}
