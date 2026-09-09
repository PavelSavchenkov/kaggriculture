#include "supply_bounds.hpp"
#include <iostream>

using namespace kag;
using namespace day_solver;

DayProblem one_crop(int radius, int purchase_hour, int initial_stock = 0) {
    DayProblem p;
    ManagedTileState crop; crop.kind = ManagedTileKind::CROP; crop.crop = WHEAT; crop.stored_units = 1;
    p.start.managed_tiles.push_back({int8_t(4 - radius), 4, crop});
    TileWorkAction fertilize; fertilize.op = OP_FERTILIZE;
    p.tile_work.push_back({0, {fertilize}}); p.start.shed[FERTILIZER] = initial_stock;
    p.market_plan.push_back({int8_t(purchase_hour), 0, M_BUY_PRODUCT, FERTILIZER, 1, 0});
    return p;
}

bool engine_fertilizes(int radius, int purchase_hour, int hours) {
    Config config; config.weed_chance = 0; config.starting_money = 1000000;
    Sim sim(config); auto& farm = sim.st.farms[0];
    auto& tile = farm.tiles[4][4 - radius];
    tile.kind = T_PLANT; tile.what = WHEAT; tile.yield_units = 1; tile.planted_day = 0;
    const int cell = 44 - radius; const uint64_t bit = uint64_t{1} << cell;
    farm.empty_mask[0] &= ~bit; farm.plant_mask[0] |= bit;
    for (int h = 0; h < hours; ++h) {
        Action own, other;
        if (h == purchase_hour) { own.n_orders = 1; own.orders[0] = {M_BUY_PRODUCT, FERTILIZER, 1}; }
        if (h == purchase_hour + 1) own.units[0] = {OP_PICKUP, FERTILIZER, 1};
        if (h > purchase_hour + 1 && h <= purchase_hour + 1 + radius) own.units[0].op = OP_WEST;
        if (h == purchase_hour + 2 + radius) own.units[0].op = OP_FERTILIZE;
        own.finalize(); other.finalize(); sim.step(own, other);
    }
    return sim.st.farms[0].tiles[4][4 - radius].fertilized_until_day > 0;
}

int main() {
    try {
        int controls = 0;
        for (int hours : {23, 24}) for (int radius = 0; radius <= 4; ++radius) {
            const int last_buy = hours - radius - 3;
            auto valid = one_crop(radius, last_buy);
            const auto a = labor::supply_bound(valid, labor::earliest_menu(valid, hours), hours);
            if (!engine_fertilizes(radius, last_buy, hours) || a.workers != 1 || a.missing)
                throw std::runtime_error("last reachable input incorrectly rejected");
            auto late = one_crop(radius, last_buy + 1);
            const auto b = labor::supply_bound(late, labor::earliest_menu(late, hours), hours);
            if (engine_fertilizes(radius, last_buy + 1, hours) || !b.missing)
                throw std::runtime_error("late input boundary not detected");
            late.start.shed[FERTILIZER] = 1;
            if (labor::supply_bound(late, labor::earliest_menu(late, hours), hours).missing)
                throw std::runtime_error("unused late purchase rejected despite initial supply");
            late.start.shed[FERTILIZER] = 0;
            TileWorkAction collect; collect.op = OP_COLLECT_FERTILIZER; collect.output_item = FERTILIZER; collect.output_quantity = 1;
            late.tile_work.push_back({0, {collect}});
            if (labor::supply_bound(late, labor::earliest_menu(late, hours), hours).missing)
                throw std::runtime_error("potential field output was not relaxed to dawn");
            controls += 4;
        }
        auto p = one_crop(0, 21); p.start.managed_tiles.push_back({5, 4, p.start.managed_tiles[0].state});
        p.tile_work.push_back({1, p.tile_work[0].actions}); p.market_plan[0].quantity = 2;
        auto bound = labor::supply_bound(p, labor::earliest_menu(p));
        if (bound.workers != 2 || bound.missing) throw std::runtime_error("late action capacity not aggregated");
        p.market_plan[0].quantity = 1;
        if (!labor::supply_bound(p, labor::earliest_menu(p)).missing) throw std::runtime_error("total input shortage missed");
        std::cout << "Supply controls passed: " << controls + 2 << " boundary, early-stock, local-production and shared-capacity checks, with independent engine timing at both horizons.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
