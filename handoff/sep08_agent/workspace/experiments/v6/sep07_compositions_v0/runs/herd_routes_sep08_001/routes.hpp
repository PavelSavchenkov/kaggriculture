#pragma once
#include "agents/common/api/agent_api.hpp"
#include <algorithm>
#include <array>
#include <vector>

namespace compositions::herd_routes_sep08 {
using namespace kag;

inline int distance(int x,int y,int a,int b){return std::abs(x-a)+std::abs(y-b);}
inline int shed_distance(int x,int y){return distance(x,y,std::clamp(x,4,5),std::clamp(y,4,5));}
inline UnitAction toward(int x,int y,int a,int b){
    if(x!=a)return {uint8_t(x<a?OP_EAST:OP_WEST),0,1};
    if(y!=b)return {uint8_t(y<b?OP_SOUTH:OP_NORTH),0,1};
    return {};
}
inline bool produces(const Tile& tile,int day){
    const auto& a=ANIMALS[tile.what-GOOSE];const int age=day-tile.planted_day-a.first_yield_day;
    return age>=0 && age%a.interval==0;
}
inline bool care(const Tile& tile,int day){
    if(tile.cared_today)return false;
    bool future=false;for(int d=day+2;d<30;++d)future|=produces(tile,d);
    return future && (tile.pending_care_bonus<ANIMALS[tile.what-GOOSE].max_held-1 || produces(tile,day+1));
}

class Routes {
    std::array<std::array<int,100>,MAX_UNITS> tiles_{};
    std::array<int,MAX_UNITS> lengths_{},cursor_{};
    int day_=-1,units_=0;
    bool active_=false;

    int work(const Tile& t,int day) const {
        return int(!t.fed_today)+int(care(t,day))+int(t.yield_units>0)+int(t.fertilizer_available);
    }
    int cost(const agent::AgentObservation& o,int unit,const std::array<int,100>& route,int size) const {
        const auto& f=o.self();int feed=0;
        for(int i=0;i<size;++i)feed+=!f.tiles[route[i]/10][route[i]%10].fed_today;
        int x=f.pos_x[unit],y=f.pos_y[unit],time=0;
        if(feed>o.own.inv[unit][WHEAT]){time+=shed_distance(x,y)+1;x=std::clamp(x,4,5);y=std::clamp(y,4,5);}
        for(int i=0;i<size;++i){int cell=route[i];time+=distance(x,y,cell%10,cell/10)+work(f.tiles[cell/10][cell%10],o.day);x=cell%10;y=cell/10;}
        return time+shed_distance(x,y)+1;
    }
    void assign(const agent::AgentObservation& o){
        lengths_.fill(0);cursor_.fill(0);units_=o.self().n_units;
        std::vector<int> cells;
        for(int c=0;c<100;++c)if(o.self().tiles[c/10][c%10].has_animal)cells.push_back(c);
        std::stable_sort(cells.begin(),cells.end(),[](int a,int b){return shed_distance(a%10,a/10)>shed_distance(b%10,b/10);});
        for(int cell:cells){
            int best=-1,position=-1,best_cost=1000000;
            for(int u=0;u<units_;++u)for(int pos=0;pos<=lengths_[u];++pos){
                auto route=tiles_[u];for(int i=lengths_[u];i>pos;--i)route[i]=route[i-1];route[pos]=cell;
                int score=cost(o,u,route,lengths_[u]+1);
                if(score<best_cost){best_cost=score;best=u;position=pos;}
            }
            if(best<0)std::abort();
            for(int i=lengths_[best];i>position;--i)tiles_[best][i]=tiles_[best][i-1];
            tiles_[best][position]=cell;++lengths_[best];
        }
    }
public:
    int active_turns=0;
    uint32_t active_days=0;
    void reset(){day_=-1;units_=0;active_=false;active_turns=0;active_days=0;lengths_.fill(0);cursor_.fill(0);}
    bool apply(const agent::AgentObservation& o,Action& action,int crop_end_day){
        if(o.day!=day_){day_=o.day;active_=false;units_=0;}
        if(o.day<crop_end_day || o.day>=29)return false;
        bool crops=false;int animals=0;
        for(int c=0;c<100;++c){const auto& t=o.self().tiles[c/10][c%10];crops|=t.kind==T_PLANT;animals+=t.has_animal;}
        if(crops || !animals)return false;
        if(!active_){if(o.hour<3 || o.hour>6)return false;assign(o);active_=true;}
        if(units_!=o.self().n_units)assign(o);
        ++active_turns;active_days|=uint32_t{1}<<o.day;
        const auto& f=o.self();int available=o.own.shed[WHEAT],space=100-o.own.shed_total;
        for(int u=0;u<f.n_units;++u){
            auto& a=action.units[u];a={};
            while(cursor_[u]<lengths_[u]){
                int c=tiles_[u][cursor_[u]];const auto& t=f.tiles[c/10][c%10];
                if(t.has_animal && work(t,o.day)>0)break;++cursor_[u];
            }
            int needed=0;for(int i=cursor_[u];i<lengths_[u];++i){int c=tiles_[u][i];const auto& t=f.tiles[c/10][c%10];needed+=t.has_animal&&!t.fed_today;}
            const int x=f.pos_x[u],y=f.pos_y[u];const bool shed=shed_distance(x,y)==0;
            const int carried=o.own.inv[u][WHEAT];
            if(shed){
                // Return excess wheat so other assigned routes can withdraw it.
                if(carried>needed && space>0){int n=std::min(carried-needed,space);a={OP_PLACE,WHEAT,n};space-=n;available+=n;continue;}
                if(carried<needed && available>0){int n=std::min(needed-carried,available);a={OP_PICKUP,WHEAT,n};available-=n;space+=n;continue;}
                int item=-1;
                for(int p=0;p<N_PRODUCTS;++p)if(p!=WHEAT && o.own.inv[u][p]>0 && (item<0 || o.own.inv[u][p]*o.market.prices[p]>o.own.inv[u][item]*o.market.prices[item]))item=p;
                if(item>=0 && space>0){int n=std::min<int>(o.own.inv[u][item],space);a={OP_PLACE,uint8_t(item),n};space-=n;continue;}
            }
            if(cursor_[u]>=lengths_[u]){
                bool cargo=false;for(int p=0;p<N_PRODUCTS;++p)cargo|=o.own.inv[u][p]>0;
                if(cargo)a=toward(x,y,std::clamp(x,4,5),std::clamp(y,4,5));
                continue;
            }
            // A route takes its complete remaining wheat requirement in one trip.
            if(carried<needed && !shed){a=toward(x,y,std::clamp(x,4,5),std::clamp(y,4,5));continue;}
            const int cell=tiles_[u][cursor_[u]];const auto& t=f.tiles[cell/10][cell%10];
            if(x!=cell%10 || y!=cell/10){a=toward(x,y,cell%10,cell/10);continue;}
            if(!t.fed_today && carried>0)a={OP_FEED,0,1};
            else if(care(t,o.day))a={OP_CARE,0,1};
            else if(t.yield_units>0)a={OP_HARVEST,0,1};
            else if(t.fertilizer_available)a={OP_COLLECT_FERTILIZER,0,1};
            else if(!t.fed_today)a=toward(x,y,std::clamp(x,4,5),std::clamp(y,4,5));
        }
        return true;
    }
};

template<class Base,int Mode,int CropEndDay=25> class Agent:public Base {
    Routes routes_;
public:
    static agent::AgentInfo info(){return {"herd_routes"};}
    void reset(const agent::AgentInit& init){Base::reset(init);routes_.reset();}
    uint32_t routed_days() const{return routes_.active_days;}
    int routed_turns() const{return routes_.active_turns;}
    void act(const agent::AgentObservation& o,const agent::DecisionBudget&,Action& a){
        typename Base::MarketContext context;Base::plan_units(o,a,context);
        if constexpr(Mode)routes_.apply(o,a,CropEndDay);
        Base::plan_market(o,a,context);
    }
};
}
