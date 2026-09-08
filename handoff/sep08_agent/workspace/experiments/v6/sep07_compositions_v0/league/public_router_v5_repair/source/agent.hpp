#pragma once
#include "../../public_router_v5/source/agent.hpp"

namespace kag::agents::public_router_v5_repair {
class Agent {
    compositions::public_router_v5::Agent base_;
public:
    static kag::agent::AgentInfo info(){return {"public_router_v5_repair"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if(o.step<0 || o.step>=719)return;
        const auto& farm=o.self();
        for(int u=0;u<action.n_units;++u) {
            const int x=farm.pos_x[u],y=farm.pos_y[u];
            if(x<0 || x>=kag::BOARD || y<0 || y>=kag::BOARD || farm.tiles[y][x].kind!=kag::T_WEED)continue;
            const auto op=action.units[u].op;
            if(op==kag::OP_PLANT || op==kag::OP_BUILD_COOP || op==kag::OP_BUILD_PASTURE || op==kag::OP_PASS || op==kag::OP_WATER)
                action.units[u]={kag::OP_DIG,0,1};
        }
        action.finalize();
    }
};
}
