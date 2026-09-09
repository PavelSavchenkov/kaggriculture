#pragma once
#include "model.hpp"
#include "../../include/guarded_day.hpp"
#include "agents/common/runtime/observation_builder.hpp"

namespace compositions::day_programs {
struct Program {
    GuardedDay start;
    std::array<TileKey,100> end;
};

inline bool shape(const TileKey& a,const TileKey& b) {
    return a[0]==b[0] && a[1]==b[1] && a[2]==b[2];
}

inline int skip_redundant(const kag::agent::AgentObservation& o,kag::Action& action) {
    int skipped=0;
    for(int u=0;u<action.n_units;++u) {
        auto& a=action.units[u];
        const auto& tile=o.self().tiles[o.self().pos_y[u]][o.self().pos_x[u]];
        const bool empty_harvest=a.op==kag::OP_HARVEST && (tile.kind==kag::T_PLANT || tile.has_animal) && tile.yield_units==0;
        const bool empty_collection=a.op==kag::OP_COLLECT_FERTILIZER && tile.has_animal && !tile.fertilizer_available;
        const bool already_done=(a.op==kag::OP_FEED && tile.has_animal && tile.fed_today) ||
            (a.op==kag::OP_CARE && tile.has_animal && tile.cared_today) ||
            (a.op==kag::OP_WATER && tile.kind==kag::T_PLANT && tile.watered_today);
        if(empty_harvest || empty_collection || already_done){a={};++skipped;}
    }
    action.finalize();return skipped;
}

template<class Base,int Mode> class Agent {
    Base base_;
    ObservationModel model_;
    std::vector<Program> programs_;
    std::array<std::vector<int>,30> index_;
    std::array<kag::Action,24> selected_;
    int active_=-1;
    uint32_t matched_days_=0;
    int active_hours_=0,abandoned_=0,screened_=0,rejected_=0,skipped_=0;

    bool entry(const Program& p,const kag::agent::AgentObservation& o) const {
        if constexpr(Mode==1)return p.start.matches(o);
        const auto& own=o.self();
        if(own.n_units!=1 || own.n_quadrants!=p.start.quadrants || own.pos_x[0]!=4 || own.pos_y[0]!=4)return false;
        for(int i=0;i<kag::N_ITEMS;++i)if(o.own.inv[0][i])return false;
        for(int cell=0;cell<100;++cell)if(p.start.check[cell]) {
            const auto current=tile_key(own.tiles[cell/10][cell%10],o.day);
            if constexpr(Mode==2){if(current!=p.start.tiles[cell])return false;}
            else if(!shape(current,p.start.tiles[cell]))return false;
        }
        return true;
    }

    bool screen(const Program& program,const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,uint64_t& expansions) {
        ++screened_;selected_=program.start.plan.actions;
        auto sim=model_.make(o);
        for(int hour=0;hour<24;++hour) {
            if(expansions>=budget.max_expansions || (hour%4==0 && (budget.soft_expired() || budget.hard_expired())))return false;
            ++expansions;
            auto observation=kag::agent::runtime::make_observation(sim,o.player);
            auto& action=selected_[hour];
            if(action.n_units!=observation.self().n_units)return false;
            if constexpr(Mode==3)skip_redundant(observation,action);
            kag::Action pass;pass.clear();pass.n_units=sim.st.farms[o.player^1].n_units;pass.finalize();
            const auto checked=o.player==0?sim.diagnose_joint_actions(action,pass):sim.diagnose_joint_actions(pass,action);
            const auto& result=checked.players[o.player];
            if(result.successful_unit_actions!=result.requested_unit_actions)return false;
            if(o.player==0)sim.step(action,pass);else sim.step(pass,action);
        }
        const auto& farm=sim.st.farms[o.player];
        for(int item=0;item<kag::N_ITEMS;++item)if(farm.discarded[item])return false;
        for(int cell=0;cell<100;++cell)if(program.start.check[cell])
            if(!shape(tile_key(farm.tiles[cell/10][cell%10],o.day+1),program.end[cell]))return false;
        return true;
    }
public:
    explicit Agent(std::vector<Program> programs):programs_(std::move(programs)) {
        for(int i=0;i<int(programs_.size());++i) {
            const int day=programs_[i].start.plan.day;
            if(day<0 || day>=29)std::abort();
            index_[day].push_back(i);
        }
    }
    void reset(const kag::agent::AgentInit& init) {
        base_.reset(init);model_.reset(init.config);active_=-1;matched_days_=0;
        active_hours_=abandoned_=screened_=rejected_=skipped_=0;
    }
    uint32_t matched_days()const{return matched_days_;}
    int active_hours()const{return active_hours_;}
    int abandoned_days()const{return abandoned_;}
    int screened_programs()const{return screened_;}
    int rejected_programs()const{return rejected_;}
    int skipped_actions()const{return skipped_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        if constexpr(Mode==0)return;
        if constexpr(Mode>=2)model_.advance(o.step);
        if(o.hour==0) {
            active_=-1;
            uint64_t expansions=0;
            for(int id:index_[o.day]) {
                if(!entry(programs_[id],o))continue;
                if constexpr(Mode>=2) {
                    if(budget.soft_expired() || budget.hard_expired() || expansions>=budget.max_expansions)break;
                    if(!screen(programs_[id],o,budget,expansions)){++rejected_;continue;}
                }else selected_=programs_[id].start.plan.actions;
                active_=id;break;
            }
        }
        if(active_<0)return;
        auto next=selected_[o.hour];
        bool valid=next.n_units==o.self().n_units;
        if constexpr(Mode>=2) {
            if(valid) {
                if constexpr(Mode==3)skip_redundant(o,next);
                auto sim=model_.make(o);
                kag::Action pass;pass.clear();pass.n_units=o.opponent().n_units;pass.finalize();
                const auto checked=o.player==0?sim.diagnose_joint_actions(next,pass):sim.diagnose_joint_actions(pass,next);
                valid=checked.players[o.player].successful_unit_actions==checked.players[o.player].requested_unit_actions;
            }
        }
        if(!valid){++abandoned_;active_=-1;return;}
        if constexpr(Mode==3)for(int u=0;u<next.n_units;++u)
            skipped_+=programs_[active_].start.plan.actions[o.hour].units[u].op!=kag::OP_PASS && next.units[u].op==kag::OP_PASS;
        matched_days_|=uint32_t{1}<<o.day;++active_hours_;action=next;action.finalize();
    }
};
}
