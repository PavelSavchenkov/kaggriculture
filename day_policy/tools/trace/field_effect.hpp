#pragma once
#include "fast_game_engine/sim.hpp"

namespace schedule_search {
// Classify an already accepted action and update the tile's occupancy in worker
// order. Inventory deposits of animals are not animal establishment jobs.
inline bool field_effect(kag::Tile& t, kag::UnitAction a) {
    using namespace kag;
    if (a.op == OP_PLACE && is_animal(a.arg)) {
        const auto kind = ANIMALS[a.arg - GOOSE].structure == ST_COOP ? T_COOP : T_PASTURE;
        if (t.kind != kind || t.has_animal) return false;
        t.has_animal = true; t.what = a.arg; return true;
    }
    if (a.op < OP_PLANT || a.op > OP_CARE) return false;
    if (a.op == OP_DIG || (a.op == OP_HARVEST && t.kind == T_PLANT && !CROPS[t.what].ongoing)) t = {};
    else if (a.op == OP_PLANT) { t = {}; t.kind = T_PLANT; t.what = a.arg; }
    else if (a.op == OP_BUILD_COOP || a.op == OP_BUILD_PASTURE) {
        t = {}; t.kind = a.op == OP_BUILD_COOP ? T_COOP : T_PASTURE;
    }
    return true;
}
}
