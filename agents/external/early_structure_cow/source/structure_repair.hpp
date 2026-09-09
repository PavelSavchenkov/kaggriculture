#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>
#include <algorithm>
#include <cstdlib>

namespace compositions::early_structure_repair {
// The inherited opening leaves the farmer idle from day1 hour9 to day end.
// Recover requested structures blocked by observed weeds during that opening.
// This is a repair of a known idle interval, not a general worker scheduler.
template<class Parent> class Policy {
    Parent parent_;
    std::array<uint8_t,100> pending_{};
    std::array<kag::UnitAction,24> route_{};
    int end_=0;
public:
    int repairs=0,conflicts=0;
    static kag::agent::AgentInfo info(){return {"early_structure_repair"};}
    void reset(const kag::agent::AgentInit& init){parent_.reset(init);pending_.fill(0);route_={};end_=0;repairs=conflicts=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        parent_.act(o,b,a);if(o.day!=1)return;
        const auto& farm=o.self();
        if(o.hour<=9)for(int u=0;u<a.n_units;++u){
            const auto op=a.units[u].op;const int x=farm.pos_x[u],y=farm.pos_y[u];
            if((op==kag::OP_BUILD_COOP || op==kag::OP_BUILD_PASTURE) && farm.tiles[y][x].kind==kag::T_WEED)
                pending_[y*10+x]=op;
        }
        if(o.hour==9 && a.units[0].op==kag::OP_PASS && o.own.inv_nkeys[0]==0){
            const int sx=farm.pos_x[0],sy=farm.pos_y[0];int x=sx,y=sy,h=9;
            auto move=[&](int tx,int ty){
                while(x!=tx){route_[h++]={uint8_t(x<tx?kag::OP_EAST:kag::OP_WEST),0,1};x+=x<tx?1:-1;}
                while(y!=ty){route_[h++]={uint8_t(y<ty?kag::OP_SOUTH:kag::OP_NORTH),0,1};y+=y<ty?1:-1;}
            };
            for(;;){
                int selected=-1,best=1000;
                for(int c=0;c<100;++c)if(pending_[c]){
                    const int tx=c%10,ty=c/10;const auto kind=farm.tiles[ty][tx].kind;
                    if(kind!=kag::T_WEED && kind!=kag::T_EMPTY)continue;
                    const int distance=std::abs(x-tx)+std::abs(y-ty);
                    const int need=distance+1+(kind==kag::T_WEED)+std::abs(sx-tx)+std::abs(sy-ty);
                    if(h+need<=24 && distance<best){selected=c;best=distance;}
                }
                if(selected<0)break;
                move(selected%10,selected/10);
                if(farm.tiles[y][x].kind==kag::T_WEED)route_[h++]={kag::OP_DIG,0,1};
                route_[h++]={pending_[selected],0,1};pending_[selected]=0;++repairs;
            }
            if(h>9){move(sx,sy);end_=h;}
        }
        if(o.hour>=9 && o.hour<end_){
            if(a.units[0].op!=kag::OP_PASS){++conflicts;return;}
            auto command=route_[o.hour];const auto kind=farm.tiles[farm.pos_y[0]][farm.pos_x[0]].kind;
            if(command.op==kag::OP_DIG && kind!=kag::T_WEED)command={};
            if((command.op==kag::OP_BUILD_COOP || command.op==kag::OP_BUILD_PASTURE) && kind!=kag::T_EMPTY)command={};
            a.units[0]=command;a.finalize();
        }
    }
};
}
