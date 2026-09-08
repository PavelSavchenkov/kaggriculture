#pragma once
#include "../day_programs_sep08_001/days.hpp"
#include <cmath>

namespace compositions::day_program_value {
using day_programs::Program;
using day_programs::shape;
using day_programs::skip_redundant;

// Compare identical observation-built forecasts. This is conditional on rival
// PASS, no new shops and no random weeds, not a prediction of hidden state.
template<class Base,int Days> class Agent {
    Base base_;
    day_programs::ObservationModel model_;
    std::vector<Program> programs_;
    std::array<std::vector<int>,30> index_;
    std::array<kag::Action,24> selected_;
    int active_=-1;
    uint32_t matched_days_=0;
    int active_hours_=0,abandoned_=0,screened_=0,rejected_=0,skipped_=0;
    int value_rejected_=0;
    uint64_t forecast_steps_=0;
    double predicted_gain_=0;

    bool entry(const Program& p,const kag::agent::AgentObservation& o) const {
        const auto& own=o.self();
        if(own.n_units!=1 || own.n_quadrants!=p.start.quadrants || own.pos_x[0]!=4 || own.pos_y[0]!=4)return false;
        for(int i=0;i<kag::N_ITEMS;++i)if(o.own.inv[0][i])return false;
        for(int cell=0;cell<100;++cell)if(p.start.check[cell] && !shape(tile_key(own.tiles[cell/10][cell%10],o.day),p.start.tiles[cell]))return false;
        return true;
    }

    double terminal_value(const kag::Sim& sim,const kag::agent::AgentObservation& anchor) const {
        const auto& farm=sim.st.farms[anchor.player];
        double value=farm.money;
        if(sim.st.done)return value;
        // Mark remaining products at the same observed prices in both forecasts.
        // This deliberately does not claim an exact value for unfinished biology.
        for(int item=0;item<kag::N_PRODUCTS;++item) {
            int units=farm.shed[item];
            for(int u=0;u<farm.n_units;++u)units+=farm.inv[u][item];
            for(int cell=0;cell<100;++cell) {
                const auto& t=farm.tiles[cell/10][cell%10];
                if(t.kind==kag::T_PLANT && t.what==item)units+=t.yield_units;
                if(t.has_animal && kag::ANIMALS[t.what-kag::GOOSE].product==item)units+=t.yield_units;
            }
            value+=units*kag::market_price(item,anchor.market.inventory[item]);
        }
        for(int c=0;c<kag::N_CROPS;++c)value+=farm.seeds[c]*kag::CROPS[c].seed;
        return value;
    }

    bool forecast(const Program* program,const kag::agent::AgentObservation& o,
                  const kag::Action& original,const kag::agent::DecisionBudget& budget,
                  uint64_t& expansions,double& value,std::array<kag::Action,24>& plan) {
        auto sim=model_.make(o);
        auto policy=base_; // base_ already processed this turn; reuse original first.
        if(program)plan=program->start.plan.actions;
        const int end=std::min(719,o.step+24*Days);
        for(int step=o.step;step<end && !sim.st.done;++step) {
            if(expansions>=budget.max_expansions || ((step-o.step)%4==0 && (budget.soft_expired() || budget.hard_expired())))return false;
            ++expansions;++forecast_steps_;
            // Hold the revealed shop set constant throughout both continuations.
            sim.st.n_shops=o.n_shops;
            std::copy_n(o.shops,o.n_shops,sim.st.shops);
            auto observation=kag::agent::runtime::make_observation(sim,o.player);
            kag::Action action=original;
            if(step!=o.step)policy.act(observation,budget,action);
            const int offset=step-o.step;
            const bool use_program=program && offset<24;
            if(use_program) {
                action=plan[offset];
                if(action.n_units!=observation.self().n_units)return false;
                skip_redundant(observation,action);plan[offset]=action;
            }
            kag::Action pass;pass.clear();pass.n_units=sim.st.farms[o.player^1].n_units;pass.finalize();
            if(use_program) {
                const auto d=o.player==0?sim.diagnose_joint_actions(action,pass):sim.diagnose_joint_actions(pass,action);
                if(d.players[o.player].successful_unit_actions!=d.players[o.player].requested_unit_actions)return false;
            }
            if(o.player==0)sim.step(action,pass);else sim.step(pass,action);
            if(program && offset==23) {
                const auto& own=sim.st.farms[o.player];
                for(int item=0;item<kag::N_ITEMS;++item)if(own.discarded[item])return false;
                for(int cell=0;cell<100;++cell)if(program->start.check[cell] && !shape(tile_key(own.tiles[cell/10][cell%10],o.day+1),program->end[cell]))return false;
            }
        }
        value=terminal_value(sim,o);
        return std::isfinite(value);
    }
public:
    explicit Agent(std::vector<Program> programs):programs_(std::move(programs)) {
        for(int i=0;i<int(programs_.size());++i) {
            const int day=programs_[i].start.plan.day;
            if(day<0 || day>=29)std::abort();index_[day].push_back(i);
        }
    }
    void reset(const kag::agent::AgentInit& init) {
        base_.reset(init);model_.reset(init.config);active_=-1;matched_days_=0;
        active_hours_=abandoned_=screened_=rejected_=skipped_=value_rejected_=0;
        forecast_steps_=0;predicted_gain_=0;
    }
    uint32_t matched_days()const{return matched_days_;}
    int active_hours()const{return active_hours_;}
    int abandoned_days()const{return abandoned_;}
    int screened_programs()const{return screened_;}
    int rejected_programs()const{return rejected_;}
    int skipped_actions()const{return skipped_;}
    int value_rejected_programs()const{return value_rejected_;}
    uint64_t forecast_steps()const{return forecast_steps_;}
    double predicted_gain()const{return predicted_gain_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& action) {
        base_.act(o,budget,action);
        model_.advance(o.step);
        if(o.hour==0) {
            active_=-1;
            bool any=false;for(int id:index_[o.day])any|=entry(programs_[id],o);
            if(!any || budget.soft_expired() || budget.hard_expired() || budget.max_expansions==0)return;
            uint64_t expansions=0;
            double original_value=0,best_value=0;
            std::array<kag::Action,24> plan;
            if(!forecast(nullptr,o,action,budget,expansions,original_value,plan))return;
            best_value=original_value;
            for(int id:index_[o.day]) {
                if(!entry(programs_[id],o))continue;
                if(budget.soft_expired() || budget.hard_expired() || expansions>=budget.max_expansions)break;
                ++screened_;double value=0;
                if(!forecast(&programs_[id],o,action,budget,expansions,value,plan)){++rejected_;continue;}
                if(value<=best_value){++value_rejected_;continue;}
                best_value=value;active_=id;selected_=plan;
            }
            if(active_>=0)predicted_gain_+=best_value-original_value;
        }
        if(active_<0)return;
        auto next=selected_[o.hour];
        bool valid=next.n_units==o.self().n_units;
        if(valid) {
            skip_redundant(o,next);
            auto sim=model_.make(o);kag::Action pass;pass.clear();pass.n_units=o.opponent().n_units;pass.finalize();
            const auto d=o.player==0?sim.diagnose_joint_actions(next,pass):sim.diagnose_joint_actions(pass,next);
            valid=d.players[o.player].successful_unit_actions==d.players[o.player].requested_unit_actions;
        }
        if(!valid){++abandoned_;active_=-1;return;}
        for(int u=0;u<next.n_units;++u)skipped_+=programs_[active_].start.plan.actions[o.hour].units[u].op!=kag::OP_PASS && next.units[u].op==kag::OP_PASS;
        matched_days_|=uint32_t{1}<<o.day;++active_hours_;action=next;action.finalize();
    }
};
}
