#include "../../source/shared_vector.hpp"
#pragma once
#include "planned_opening.hpp"

namespace catalog_early_structure_cow_parent_sale {
using TileKey=std::array<int,12>;

inline TileKey tile_key(const kag::Tile& tile,int day) {
    TileKey key{};key[0]=tile.kind;key[1]=key[2]=-1;
    if(tile.kind==kag::T_PLANT || tile.has_animal) {
        key[3]=day-tile.planted_day;key[4]=tile.yield_units;key[5]=tile.consecutive_dry;
    }
    if(tile.kind==kag::T_PLANT) {
        key[1]=tile.what;key[7]=std::max(0,tile.fertilized_until_day-day+1);key[8]=tile.watered_today;
    }
    if(tile.has_animal) {
        key[2]=tile.what;key[6]=tile.pending_care_bonus;key[9]=tile.fed_today;
        key[10]=tile.cared_today;key[11]=tile.fertilizer_available;
    }
    return key;
}

struct GuardedDay {
    DayPlan plan;
    std::array<TileKey,100> tiles;
    std::array<bool,100> check;
    std::array<int,kag::N_ITEMS> shed;
    std::array<int,kag::N_CROPS> seeds;
    int quadrants;
    bool matches(const kag::agent::AgentObservation& o) const {
        const auto& own=o.self();
        if(own.n_units!=1 || own.n_quadrants!=quadrants || own.pos_x[0]!=4 || own.pos_y[0]!=4)return false;
        for(int i=0;i<kag::N_ITEMS;++i)if(o.own.shed[i]!=shed[i] || o.own.inv[0][i])return false;
        for(int i=0;i<kag::N_CROPS;++i)if(o.own.seeds[i]!=seeds[i])return false;
        for(int cell=0;cell<100;++cell)
            if(check[cell] && tile_key(own.tiles[cell/10][cell%10],o.day)!=tiles[cell])return false;
        return true;
    }
};

// Check the source physical starting contract at day start, then commit to a
// complete route day. Untouched empty/weed/locked tiles are irrelevant to its
// field work. This does not certify future cash, prices or opponent behavior.
template<class Base,bool OpeningGate=true> class GuardedOpeningAgent {
    Base base_;
    SharedVector<GuardedDay> days_;
    std::array<std::vector<int>,30> index_;
    bool selected_=false;
    int selected_day_=-1;
    uint32_t matched_days_=0;
public:
    explicit GuardedOpeningAgent(std::vector<GuardedDay> days,Base base=Base{}):base_(std::move(base)),days_(std::move(days)) {
        for(int i=0;i<int(days_.size());++i) {
            const int day=days_[i].plan.day;
            if(day<0 || day>=29)std::abort();
            if constexpr(OpeningGate)if(day==0 || day==18)std::abort();
            index_[day].push_back(i);
        }
    }
    static kag::agent::AgentInfo info() {return {"guarded_opening"};}
    void reset(const kag::agent::AgentInit& init) {
        base_.reset(init);selected_=!OpeningGate;selected_day_=-1;matched_days_=0;
    }
    uint32_t matched_days() const {return matched_days_;}
    const Base& base_agent() const {return base_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if constexpr(OpeningGate)if(o.step==1)selected_=o.opponent().n_units==1;
        if(o.hour==0) {
            selected_day_=-1;
            if(selected_)for(int id:index_[o.day])if(days_[id].matches(o)){selected_day_=id;break;}
        }
        if(selected_day_<0)return;
        matched_days_|=uint32_t(1)<<o.day;
        action=days_[selected_day_].plan.actions[o.hour];
        for(int u=action.n_units;u<o.self().n_units;++u)action.units[u]={};
        action.n_units=o.self().n_units;action.finalize();
    }
};
template<class Base> using GuardedDayAgent=GuardedOpeningAgent<Base,false>;
}
