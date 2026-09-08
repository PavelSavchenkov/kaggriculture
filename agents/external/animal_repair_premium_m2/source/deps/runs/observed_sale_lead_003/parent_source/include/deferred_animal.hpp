#pragma once
#include "guarded_day.hpp"
#include "sampled_animal_value.hpp"

namespace catalog_animal_repair_premium_m2_sale {
// One reserved animal investment. A compiled entry day includes the new animal,
// structure, feed, care and all other farm jobs. Waiting preserves the empty
// structure and can select a later entry. It never pretends deletion is a buy.
struct AnimalEntry {
    int item=kag::GOOSE;
    GuardedDay day;
    int model=0;
};
struct AnimalInvestmentModel {FarmFlowPlan flows;Service service;bool force_entry_service=true;};

template<class Base> class DeferredAnimalAgent {
    Base base_;
    int purchase_step_,purchase_order_,cell_,original_;
    std::vector<AnimalEntry> entries_;
    std::vector<AnimalInvestmentModel> models_;
    int requested_day_,requested_item_,selected_=-1;
    bool use_day_=false,parent_mode_=false;
    bool relative_=true;
    double operation_cost_=0,minimum_gain_=0;
    int shop_samples_=0;
    int decisions_=0,waits_=0;
    std::array<double,4> scores_{};
public:
    DeferredAnimalAgent(int purchase_step,int purchase_order,int cell,
                        std::vector<AnimalEntry> entries={},int day=30,int item=kag::GOOSE,
                        std::vector<AnimalInvestmentModel> models={},bool relative=true,double operation_cost=0,double minimum_gain=0,
                        int original=kag::GOOSE,int shop_samples=0)
        :purchase_step_(purchase_step),purchase_order_(purchase_order),cell_(cell),original_(original),
         entries_(std::move(entries)),models_(std::move(models)),requested_day_(day),requested_item_(item),
         relative_(relative),operation_cost_(operation_cost),minimum_gain_(minimum_gain),shop_samples_(shop_samples){if(!kag::is_animal(original_) || shop_samples<0 || shop_samples>64 || (shop_samples!=1 && shop_samples%8))std::abort();}
    static kag::agent::AgentInfo info(){return {"deferred_animal"};}
    void reset(const kag::agent::AgentInit& init){base_.reset(init);selected_=-1;use_day_=parent_mode_=false;decisions_=waits_=0;scores_.fill(0);}
    int chosen() const {return selected_<0?-1:entries_[selected_].item;}
    bool entered() const {return selected_>=0;}
    int entry_day() const {return selected_<0?30:entries_[selected_].day.plan.day;}
    int decisions() const {return decisions_;}
    int waits() const {return waits_;}
    const auto& scores() const {return scores_;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& budget,kag::Action& a) {
        base_.act(o,budget,a);
        if(requested_day_<0 || parent_mode_)return; // Exact parent control.
        if(o.hour==0) {
            use_day_=false;
            if(selected_<0 && !models_.empty()) {
                int context=-1;
                for(const auto& entry:entries_)if(entry.day.plan.day==o.day && entry.day.matches(o)){context=entry.model;break;}
                if(context<0 && o.day==purchase_step_/24){parent_mode_=true;return;}
                if(context>=0) {
                    ++decisions_;scores_.fill(-1e100);scores_[3]=0;
                    const auto& model=models_[context];
                    const SampledAnimalValue forecast(o,shop_samples_);
                    const auto zero=forecast.value(model.flows,Biology{},operation_cost_);
                    const double baseline=relative_?zero.margin():zero.own;
                    double best=minimum_gain_;int winner=-1;
                    // Receding horizon: delayed choices are valued with today's
                    // facts, then reconsidered from newly observed shops later.
                    // These are heuristic forecasts, not known future prices.
                    for(int i=0;i<int(entries_.size());++i) {
                        const auto& entry=entries_[i];const int day=entry.day.plan.day;
                        if(entry.model!=context || day<o.day)continue;
                        const auto biology=investment_biology(entry.item,day,model.service,model.force_entry_service);
                        const auto value=forecast.value(model.flows,biology,operation_cost_);
                        const double gain=(relative_?value.margin():value.own)-baseline;
                        if(day==o.day)scores_[entry.item-kag::GOOSE]=gain;
                        else scores_[3]=std::max(scores_[3],gain);
                        if(gain>best){best=gain;winner=i;}
                    }
                    if(winner>=0 && entries_[winner].day.plan.day==o.day){selected_=winner;use_day_=true;}
                    else ++waits_;
                }
            } else if(selected_<0 && o.day==requested_day_)
                for(int i=0;i<int(entries_.size());++i)
                    if(entries_[i].item==requested_item_ && entries_[i].day.plan.day==o.day && entries_[i].day.matches(o)) {
                        selected_=i;use_day_=true;break;
                    }
        }
        if(o.step==purchase_step_) {
            if(purchase_order_>=a.n_orders || a.orders[purchase_order_].op!=kag::M_BUY_ANIMAL ||
               a.orders[purchase_order_].item!=original_ || a.orders[purchase_order_].n!=1)std::abort();
            a.orders[purchase_order_]={}; // Preserve simultaneous market slots.
        }
        for(int u=0;u<a.n_units;++u) {
            auto& action=a.units[u];
            const int cell=o.self().pos_y[u]*10+o.self().pos_x[u];
            if(o.day==purchase_step_/24 && action.op==kag::OP_PICKUP && action.arg==original_ && !o.own.shed[original_])action={};
            if(cell!=cell_)continue;
            if(o.day==purchase_step_/24 && action.op==kag::OP_PLACE && action.arg==original_)action={};
            const auto& tile=o.self().tiles[cell/10][cell%10];
            if(!tile.has_animal && (tile.kind==kag::T_COOP || tile.kind==kag::T_PASTURE) &&
               (action.op==kag::OP_FEED || action.op==kag::OP_CARE || action.op==kag::OP_COLLECT_FERTILIZER || action.op==kag::OP_HARVEST))action={};
        }
        if(use_day_) {
            a=entries_[selected_].day.plan.actions[o.hour];
            for(int u=a.n_units;u<o.self().n_units;++u)a.units[u]={};
            a.n_units=o.self().n_units;
        } else if(selected_>=0 && entries_[selected_].item!=original_) {
            const int from=kag::ANIMALS[original_-kag::GOOSE].product;
            const int to=kag::ANIMALS[entries_[selected_].item-kag::GOOSE].product;
            for(int u=0;u<a.n_units;++u)
                if(a.units[u].op==kag::OP_PLACE && a.units[u].arg==from && o.own.inv[u][to]>0)a.units[u]={kag::OP_DROP,0,0};
            bool sale=false,already=false;
            for(int i=0;i<a.n_orders;++i)if(a.orders[i].op==kag::M_SELL){sale|=a.orders[i].item==from;already|=a.orders[i].item==to;}
            if(sale && !already && a.n_orders<10)a.orders[a.n_orders++]={kag::M_SELL,uint8_t(to),100};
        }
        a.finalize();
    }
};
}
