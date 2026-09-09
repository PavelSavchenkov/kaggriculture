#include "release_bounds.hpp"
#include <iostream>

using namespace kag;
using namespace day_solver;

DayProblem planting(int radius, int purchase_hour, bool land, int count = 1) {
    DayProblem p;
    for (int i = 0; i < count; ++i) {
        ManagedTile tile; tile.x = land ? 5 + radius : 4 - radius; tile.y = 4 - i;
        tile.state.kind = land ? ManagedTileKind::LOCKED : ManagedTileKind::EMPTY;
        p.start.managed_tiles.push_back(tile);
        TileWorkAction plant; plant.op = OP_PLANT; plant.arg = WHEAT;
        p.tile_work.push_back({int16_t(i), {plant}});
    }
    if (land) p.start.seeds[WHEAT] = count;
    p.market_plan.push_back({int8_t(purchase_hour), 0, uint8_t(land ? M_BUY_LAND : M_BUY_SEED), int16_t(land ? 1 : WHEAT), land ? 1 : count, 0});
    return p;
}

bool engine_plants(int radius, int purchase_hour, int hours, bool land, bool same_phase_water = false) {
    Config config; config.weed_chance = 0; config.starting_money = 1000000;
    Sim sim(config); auto& farm = sim.st.farms[0];
    if (land) farm.seeds[WHEAT] = 1;
    const int x = land ? 5 + radius : 4 - radius;
    bool planted = false;
    for (int h = 0; h < hours; ++h) {
        Action own, other; own.n_units = farm.n_units;
        if (same_phase_water && h == 0) own.orders[own.n_orders++] = {M_HIRE, 0, 1};
        if (h == purchase_hour) own.orders[own.n_orders++] = {uint8_t(land ? M_BUY_LAND : M_BUY_SEED), uint8_t(land ? 0 : WHEAT), 1};
        for (int u = 0; u < farm.n_units; ++u) {
            if (farm.pos_x[u] < x) own.units[u].op = OP_EAST;
            else if (farm.pos_x[u] > x) own.units[u].op = OP_WEST;
            else if (farm.pos_y[u] > 4) own.units[u].op = OP_NORTH;
            else if (h == purchase_hour + 1)
                own.units[u] = u == 0 ? UnitAction{OP_PLANT, WHEAT, 1} : UnitAction{OP_WATER, 0, 1};
        }
        own.finalize(); other.finalize(); sim.step(own, other);
        if (h == purchase_hour + 1) planted = farm.seeds[WHEAT] == 0;
    }
    if (same_phase_water) planted &= farm.tiles[4][x].kind == T_PLANT;
    return planted;
}

int main() {
    try {
        int controls = 0;
        for (int hours : {23, 24}) for (int radius = 0; radius <= 4; ++radius) for (bool land : {false, true}) {
            auto p = planting(radius, hours - 2, land);
            const auto valid = labor::release_bound(p, labor::earliest_menu(p, hours), hours);
            if (!engine_plants(radius, hours - 2, hours, land) || valid.workers != 1 || valid.seed_missing || valid.land_missing)
                throw std::runtime_error("prepositioned last-phase planting rejected");
            p.market_plan[0].hour = hours - 1;
            const auto late = labor::release_bound(p, labor::earliest_menu(p, hours), hours);
            if (engine_plants(radius, hours - 1, hours, land) || !(land ? late.land_missing : late.seed_missing))
                throw std::runtime_error("unusable last-phase purchase missed");
            p.market_plan.clear();
            const auto absent = labor::release_bound(p, labor::earliest_menu(p, hours), hours);
            if (!(land ? absent.land_missing : absent.seed_missing)) throw std::runtime_error("absent supply missed");
            if (land) p.start.managed_tiles[0].state.kind = ManagedTileKind::EMPTY;
            else p.start.seeds[WHEAT] = 1;
            const auto stocked = labor::release_bound(p, labor::earliest_menu(p, hours), hours);
            if (stocked.workers != 1 || stocked.seed_missing || stocked.land_missing) throw std::runtime_error("initial supply ignored");
            controls += 4;
        }
        for (int hours : {23, 24}) {
            auto p = planting(4, hours - 2, true);
            TileWorkAction water; water.op = OP_WATER; p.tile_work[0].actions.push_back(water);
            auto b = labor::release_bound(p, labor::earliest_menu(p, hours), hours);
            if (!engine_plants(4, hours - 2, hours, true, true) || b.workers != 2 || b.land_missing)
                throw std::runtime_error("same-phase consecutive tile actions rejected");
            auto menu = labor::earliest_menu(p, hours); menu.hours.fill(hours - 2);
            if (labor::release_bound(p, menu, hours).workers != 41) throw std::runtime_error("late-born travel capacity ignored");
            p = planting(0, hours - 2, false, 2);
            b = labor::release_bound(p, labor::earliest_menu(p, hours), hours);
            if (b.workers != 2 || b.seed_missing) throw std::runtime_error("shared late planting capacity missed");
            p.start.seeds[WHEAT] = 1;
            if (labor::release_bound(p, labor::earliest_menu(p, hours), hours).workers != 1) throw std::runtime_error("early seed stock ignored");
            controls += 4;
        }
        std::cout << "Release controls passed: " << controls << ", including engine timing at both horizons and same-phase plant/water.\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
