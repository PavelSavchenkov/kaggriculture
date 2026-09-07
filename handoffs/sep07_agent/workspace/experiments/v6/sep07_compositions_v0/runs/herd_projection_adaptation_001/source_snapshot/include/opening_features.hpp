#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions {
struct OpeningFeatures {
    int money=0,workers=0,land=0,farmer_x=0,farmer_y=0;
    int crops[kag::N_CROPS]{},animals[kag::N_ANIMALS]{},pastures=0,coops=0;
};
inline OpeningFeatures opening_features(const kag::agent::AgentObservation& o) {
    const auto& f=o.farms[o.player^1];
    OpeningFeatures result;
    result.money=int(f.money);result.workers=f.n_units;result.land=f.n_quadrants;
    result.farmer_x=f.pos_x[0];result.farmer_y=f.pos_y[0];
    for(const auto& row:f.tiles)for(const auto& tile:row) {
        result.pastures+=tile.kind==kag::T_PASTURE;result.coops+=tile.kind==kag::T_COOP;
        if(tile.kind==kag::T_PLANT)++result.crops[tile.what];
        if(tile.has_animal)++result.animals[tile.what-kag::GOOSE];
    }
    return result;
}
}
