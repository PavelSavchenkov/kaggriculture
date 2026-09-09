#pragma once
#include "agents/common/api/agent_api.hpp"
#include <vector>

namespace compositions::sheep_portfolio {
// Exact typed own physical/private state, excluding money. Inventory key order
// is retained; different lengths or values mean the donor state is not equal.
inline std::vector<int> physical(const kag::agent::AgentObservation& o){
    const auto& f=o.self();std::vector<int> v={f.n_units,f.n_quadrants,f.hires_today};
    for(int u=0;u<f.n_units;++u){v.push_back(f.pos_x[u]);v.push_back(f.pos_y[u]);}
    for(const auto& row:f.tiles)for(const auto& t:row){
        for(int x:{int(t.kind),int(t.kind==kag::T_PLANT || t.has_animal?t.what:0),int(t.has_animal),int(t.watered_today),int(t.fed_today),int(t.cared_today),int(t.fertilizer_available),int(t.consecutive_dry),int(t.yield_units),int(t.pending_care_bonus),int(t.planted_day),int(t.max_lifespan_step),int(t.fertilized_until_day)})v.push_back(x);
    }
    for(int x:o.own.shed)v.push_back(x);
    for(int x:o.own.seeds)v.push_back(x);
    for(int u=0;u<f.n_units;++u){v.push_back(o.own.inv_nkeys[u]);for(int k=0;k<o.own.inv_nkeys[u];++k){const int item=o.own.inv_keys[u][k];v.push_back(item);v.push_back(o.own.inv[u][item]);}}
    return v;
}
}
