#include "schedule.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <algorithm>
#include <climits>

namespace kag::day_compiler {
ResourceSchedule::ResourceSchedule() = default;
NightSettlement settle_night(const ResourceSchedule& resources,const Configuration& config) {
    NightSettlement out; out.capacity_before=config.shed_capacity;
    if(resources.hours!=24) return out;
    for(int p=0;p<N_ITEMS;++p)
        if(is_animal(p) || (resources.preserve_input_orders && (p==WHEAT || p==FERTILIZER)))
            out.locked_stock+=resources.night_input_stock[p];
    if(out.locked_stock<0 || out.locked_stock>config.shed_capacity) { out.valid=false; return out; }
    int room=config.shed_capacity-out.locked_stock,total=0;
    for(int k=0;k<resources.night_deposit_count;++k) {
        const auto deposit=resources.night_deposits[k];
        const int accepted=std::min(room,deposit.quantity);
        out.receipts[deposit.product]+=accepted; room-=accepted;
        out.unavoidable_discard+=deposit.quantity-accepted; total+=deposit.quantity;
    }
    out.capacity_before=std::max(out.locked_stock,config.shed_capacity-total);
    return out;
}

bool describe_resources(const Observation& dawn,const Action* actions,ResourceSchedule& out,const Configuration& config,bool plan_input_purchases) {
    out=ResourceSchedule(); out.start_hour=dawn.hour;
    out.hours=std::min(24,config.episode_steps-(dawn.step-dawn.hour)-1);
    if(out.start_hour<0 || out.start_hour>=out.hours) return false;
    auto unconstrained=config; unconstrained.shed_capacity=30000;
    auto sim=observed_state(dawn,unconstrained);
    int credit[N_ITEMS]{},total_credit=0;
    if(plan_input_purchases) {
        // Describe requested input pickups even when the purchase program has
        // not yet been made. Credit is only internal material accounting, never
        // money or inventory supplied to execution. Relative prefix deltas
        // cancel it; the live buyer must fund and buy every actual shortfall.
        for(int h=out.start_hour;h<out.hours;++h) for(int u=0;u<actions[h].n_units;++u) {
            const auto x=actions[h].units[u];
            if(x.op!=OP_PICKUP || (x.arg!=WHEAT && x.arg!=FERTILIZER)) continue;
            if(x.n<0 || x.n>config.shed_capacity || total_credit+x.n>20000) return false;
            credit[x.arg]+=x.n; total_credit+=x.n;
        }
        auto& farm=sim.st.farms[dawn.player];
        for(int p:{WHEAT,FERTILIZER}) farm.shed[p]+=credit[p];
        farm.shed_total+=total_credit; out.flexible_inputs=true; out.planned_input_purchases=true;
    }
    int deltas[24][N_ITEMS]{}, prefix_need[24][N_ITEMS]{}, peak[24]{};
    for(int h=out.start_hour;h<out.hours;++h) {
        out.shed_transfer_begin[h]=out.shed_transfer_count;
        auto o=agent::runtime::make_observation(sim,dawn.player);
        auto& actual=sim.st.farms[dawn.player]; actual.money=1e9;
        const auto& action=actions[h]; out.workers[h]=action; out.required_workers[h]=actual.n_units;
        if(action.n_units!=actual.n_units) return false;
        auto current=own_farm(o); const int start_total=current.shed_total;
        int start[N_ITEMS]; std::copy_n(current.shed,N_ITEMS,start);
        for(int u=0;u<action.n_units;++u) {
            auto partial=agent::runtime::make_observation(sim,dawn.player);
            partial.own.shed_total=current.shed_total;
            std::copy_n(current.shed,N_ITEMS,partial.own.shed);
            std::copy_n(current.seeds,N_CROPS,partial.own.seeds);
            for(int unit=0;unit<current.n_units;++unit) {
                std::copy_n(current.inv[unit],N_ITEMS,partial.own.inv[unit]);
                partial.own.inv_nkeys[unit]=current.inv_nkeys[unit];
                std::copy_n(current.inv_keys[unit],current.inv_nkeys[unit],partial.own.inv_keys[unit]);
            }
            for(int y=0;y<BOARD;++y) std::copy_n(current.tiles[y],BOARD,partial.farms[dawn.player].tiles[y]);
            std::copy_n(current.pos_x,MAX_UNITS,partial.farms[dawn.player].pos_x);
            std::copy_n(current.pos_y,MAX_UNITS,partial.farms[dawn.player].pos_y);
            Action one; one.n_units=current.n_units; std::fill_n(one.units,one.n_units,UnitAction{});
            one.units[u]=action.units[u]; one.finalize();
            auto next=worker_phase(partial,one,unconstrained);
            auto transfer=[&](int p,int quantity) {
                if(out.shed_transfer_count>=ResourceSchedule::MAX_SHED_TRANSFERS) return false;
                out.shed_transfers[out.shed_transfer_count++]={uint8_t(p),quantity}; return true;
            };
            for(int p=0;p<N_ITEMS;++p) if(next.shed[p]<current.shed[p])
                if(!transfer(p,int(next.shed[p])-current.shed[p])) return false;
            // DROP traverses inventory insertion order, not product order.
            for(int k=0;k<current.inv_nkeys[u];++k) {
                const int p=current.inv_keys[u][k],quantity=int(next.shed[p])-current.shed[p];
                if(quantity>0 && !transfer(p,quantity)) return false;
            }
            for(int p=0;p<N_ITEMS;++p) {
                const int delta=int(next.shed[p])-current.shed[p];
                if(delta>0) out.receipts[h][p]+=delta;
                if(delta<0) out.pickups[h][p]-=delta;
                prefix_need[h][p]=std::max(prefix_need[h][p],start[p]-int(next.shed[p]));
            }
            peak[h]=std::max(peak[h],int(next.shed_total)-start_total);
            current=next;
        }
        for(int p=0;p<N_ITEMS;++p) deltas[h][p]=int(current.shed[p])-start[p];
        if(h==23) for(int u=0;u<current.n_units;++u) for(int k=0;k<current.inv_nkeys[u];++k) {
            const int p=current.inv_keys[u][k],quantity=current.inv[u][p];
            out.night_returns[p]+=quantity;
            out.night_deposits[out.night_deposit_count++]={uint8_t(p),quantity};
        }
        for(int k=0;k<action.n_orders;++k) {
            const auto order=action.orders[k];
            if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL) out.purchases[h][order.item]+=order.n;
            else if(order.op==M_SELL && (order.item==WHEAT || order.item==FERTILIZER)) out.fixed_sales[h][order.item]+=order.n;
            else if(order.op!=M_HIRE && order.op!=M_BUY_SEED && order.op!=M_BUY_LAND && order.op!=M_NONE) return false;
        }
        if(h==23) for(int p=0;p<N_ITEMS;++p)
            out.night_input_stock[p]=std::max(0,current.shed[p]-credit[p]+out.purchases[h][p]-(p<N_PRODUCTS?out.fixed_sales[h][p]:0));
        if(dawn.player==0) sim.step(action,Action{}); else sim.step(Action{},action);
        out.shed_transfer_begin[h+1]=out.shed_transfer_count;
    }
    for(int c=0;c<100;++c) out.next_dawn_tiles[c]=sim.st.farms[dawn.player].tiles[c/10][c%10];
    int need[N_ITEMS]{};
    for(int h=out.hours-1;h>=out.start_hour;--h) {
        std::copy_n(need,N_ITEMS,out.reserve_after_market[h]);
        for(int p=0;p<N_ITEMS;++p) need[p]=std::max(prefix_need[h][p],need[p]-out.purchases[h][p]-deltas[h][p]+(p<N_PRODUCTS?out.fixed_sales[h][p]:0));
        out.capacity_after_market[h]=config.shed_capacity-(h+1<out.hours?peak[h+1]:0);
    }
    for(int p=0;p<N_ITEMS;++p) {
        const int initial_need=plan_input_purchases && (p==WHEAT || p==FERTILIZER)?prefix_need[out.start_hour][p]:need[p];
        if(dawn.own.shed[p]<initial_need) return false;
    }
    for(int index=0;index<2;++index) {
        const int p=index?FERTILIZER:WHEAT; int remaining=0;
        for(int h=out.hours-1;h>=out.start_hour;--h) {
            out.input_remaining_need[h][index]=remaining;
            out.input_next_need[h][index]=h+1<out.hours?prefix_need[h+1][p]:0;
            remaining=std::max(prefix_need[h][p],remaining-deltas[h][p]);
        }
    }
    std::copy_n(&out.input_next_need[0][0],48,&out.input_buy_need[0][0]);
    if(plan_input_purchases) for(int h=out.hours-2;h>=out.start_hour;--h) {
        int fixed=0;
        for(int k=0;k<out.workers[h+1].n_orders;++k) {
            const auto x=out.workers[h+1].orders[k];
            fixed+=x.op!=M_NONE && !(x.op==M_BUY_PRODUCT && (x.item==WHEAT || x.item==FERTILIZER));
        }
        // Leave a sale slot for funding. Wheat gets the last input slot;
        // fertilizer can be bought in the preceding less crowded market.
        for(int index=0;index<2;++index) if(fixed>=config.max_orders-1-index) {
            const int p=index?FERTILIZER:WHEAT;
            out.input_buy_need[h][index]=std::max(out.input_buy_need[h][index],out.input_buy_need[h+1][index]-deltas[h+1][p]);
        }
    }
    return true;
}
ExecutionError liquidation_orders(const Observation& o,const Farm& workers,const ResourceSchedule& resources,
                                  Action& action,const Configuration& config) {
    const int hour=o.hour;
    if(hour<0 || hour>=resources.hours || o.self().n_units!=resources.required_workers[hour]) return ExecutionError::Workforce;
    if(resources.flexible_inputs && resources.preserve_input_orders) return ExecutionError::MarketPlan;
    OrderLedger ledger(o,workers,config);
    int future_buys[N_ITEMS]; std::copy_n(resources.purchases[hour],N_ITEMS,future_buys);
    int future_sales[N_PRODUCTS]; std::copy_n(resources.fixed_sales[hour],N_PRODUCTS,future_sales);
    auto flexible=[&](int p) { return resources.flexible_inputs && (p==WHEAT || p==FERTILIZER); };
    auto reserve=[&](int p) {
        if(flexible(p)) return resources.input_remaining_need[hour][p==FERTILIZER];
        return std::max(0,resources.reserve_after_market[hour][p]-future_buys[p]+future_sales[p]);
    };
    int fixed_orders_remaining=0,input_orders_remaining=0;
    for(int k=0;k<resources.workers[hour].n_orders;++k) {
        const auto x=resources.workers[hour].orders[k];
        fixed_orders_remaining+=x.op!=M_NONE && !(x.op==M_BUY_PRODUCT && flexible(x.item));
    }
    for(int p:{WHEAT,FERTILIZER}) if(flexible(p)) {
        future_buys[p]=std::max(0,resources.input_buy_need[hour][p==FERTILIZER]-ledger.stock[p]);
        input_orders_remaining+=future_buys[p]>0;
    }
    auto sell=[&] {
        int required_space=0;
        if(resources.planned_input_purchases && ledger.orders.n_orders+fixed_orders_remaining+input_orders_remaining+1==config.max_orders) {
            int final_total=ledger.total;
            for(int p=0;p<N_ITEMS;++p) final_total+=future_buys[p]-(p<N_PRODUCTS?future_sales[p]:0);
            required_space=std::max(0,final_total-resources.capacity_after_market[hour]);
        }
        int product=-1; double value=-1;
        for(int p=0;p<N_PRODUCTS;++p) {
            if(resources.preserve_input_orders && (p==WHEAT || p==FERTILIZER)) continue;
            const int quantity=std::max(0,ledger.stock[p]-reserve(p)); if(quantity<std::max(1,required_space)) continue;
            const double receipts=transact(p,ledger.inventory[p],quantity,0).own_cash;
            if(receipts>value) { product=p; value=receipts; }
        }
        return product>=0 && ledger.append({M_SELL,uint8_t(product),ledger.stock[product]-reserve(product)});
    };
    const auto& fixed=resources.workers[hour];
    // The worker plan constrains pickup prefixes, not the prototype's early
    // purchase batches. Buy each shortfall by its actual next-worker deadline.
    for(int p:{WHEAT,FERTILIZER}) if(flexible(p)) {
        const int quantity=std::max(0,resources.input_buy_need[hour][p==FERTILIZER]-ledger.stock[p]);
        if(!quantity) continue;
        while(!ledger.append({M_BUY_PRODUCT,uint8_t(p),quantity})) {
            if(ledger.orders.n_orders>=config.max_orders) return ExecutionError::MarketSlots;
            if(!sell()) return ExecutionError::Funding;
        }
        future_buys[p]-=quantity; --input_orders_remaining;
    }
    for(int k=0;k<fixed.n_orders;++k) {
        const auto order=fixed.orders[k];
        if(order.op==M_BUY_PRODUCT && flexible(order.item)) continue;
        while(!ledger.append(order)) {
            if(ledger.orders.n_orders>=config.max_orders) return ExecutionError::MarketSlots;
            if(!sell()) return ExecutionError::Funding;
        }
        if(order.op!=M_NONE) --fixed_orders_remaining;
        if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL) future_buys[order.item]-=order.n;
        if(order.op==M_SELL) future_sales[order.item]-=order.n;
    }
    while(ledger.orders.n_orders<config.max_orders && sell()) {}
    if(ledger.total>resources.capacity_after_market[hour]) return ExecutionError::Capacity;
    for(int p=0;p<N_ITEMS;++p) {
        const int minimum=flexible(p)?resources.input_buy_need[hour][p==FERTILIZER]:resources.reserve_after_market[hour][p];
        if(ledger.stock[p]<minimum) return ExecutionError::Funding;
    }
    action=fixed; action.n_orders=ledger.orders.n_orders;
    std::copy_n(ledger.orders.orders,action.n_orders,action.orders); action.finalize();
    return ExecutionError::None;
}
}
