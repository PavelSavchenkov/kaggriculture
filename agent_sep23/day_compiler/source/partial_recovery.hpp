#pragma once
#include "compiler.hpp"

namespace kag::day_compiler {
// A failed complete repair must still record the action actually executed.
// Preserve useful current worker effects and an affordable purchase prefix;
// this does not assert that the remaining day contract can be completed.
inline void partial_recovery(const Observation& o,const DaySchedule& plan,Action& action,const Configuration& config) {
    action.clear();action.n_units=o.self().n_units;
    std::fill_n(action.units,action.n_units,UnitAction{});action.finalize();
    if(o.hour<0||o.hour>=plan.resources.hours)return;
    const auto& fixed=plan.resources.workers[o.hour];
    Action workers=action;
    std::copy_n(fixed.units,std::min(fixed.n_units,workers.n_units),workers.units);workers.finalize();
    auto observed=observed_state(o,config);
    action=observed.sanitize_solo_action(o.player,workers);
    // The engine's canonical PLACE quantity is one for animals. At a shed,
    // returning multiple carried animals must retain the requested quantity.
    for(int u=0;u<action.n_units;++u)if(action.units[u].op==OP_PLACE&&is_animal(action.units[u].arg))
        action.units[u].n=workers.units[u].n;
    action.finalize();const auto after=worker_phase(o,action,config);
    OrderLedger ledger(o,after,config);
    for(int p=CARROT;p<=FERTILIZER;++p) {
        int reserve=plan.resources.reserve_after_market[o.hour][p];
        if(p==FERTILIZER)reserve=std::max({reserve,plan.resources.input_buy_need[o.hour][1],
                                         plan.resources.input_remaining_need[o.hour][1]});
        const int spare=std::max(0,ledger.stock[p]-reserve);
        if(spare)ledger.append({M_SELL,uint8_t(p),spare});
    }
    auto affordable=[&](Order order) {
        if(order.n<=0||ledger.append(order))return;
        if(order.op!=M_BUY_PRODUCT&&order.op!=M_BUY_SEED&&order.op!=M_BUY_ANIMAL)return;
        int low=0,high=order.n-1;
        while(low<high) {
            const int n=(low+high+1)/2;auto trial=ledger;auto part=order;part.n=n;
            if(trial.append(part))low=n;else high=n-1;
        }
        if(low){order.n=low;ledger.append(order);}
    };
    for(int p:{WHEAT,FERTILIZER}) {
        const int quantity=std::max(0,plan.resources.input_buy_need[o.hour][p==FERTILIZER]-ledger.stock[p]);
        if(quantity)affordable({M_BUY_PRODUCT,uint8_t(p),quantity});
    }
    for(int k=0;k<fixed.n_orders;++k) {
        const auto order=fixed.orders[k];
        if(order.op==M_SELL||order.op==M_BUY_PRODUCT)continue;
        affordable(order);
    }
    const int missing=std::max(0,plan.resources.required_workers[o.hour]-o.self().n_units);
    for(int n=0;n<missing;++n)if(!ledger.append({M_HIRE,0,1}))break;
    action.n_orders=ledger.orders.n_orders;
    std::copy_n(ledger.orders.orders,action.n_orders,action.orders);action.finalize();
}
}
