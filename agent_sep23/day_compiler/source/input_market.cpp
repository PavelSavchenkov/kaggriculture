#include "input_market.hpp"
#include "biology.hpp"
#include <algorithm>

namespace kag::day_compiler {
namespace {
bool early_sale(const Action& program,int slot) {
    const auto order=program.orders[slot];
    if(order.op!=M_SELL || order.item==WHEAT) return false;
    if(order.item!=FERTILIZER) return true;
    for(int k=0;k<slot;++k)
        if(program.orders[k].op==M_BUY_PRODUCT && program.orders[k].item==FERTILIZER) return false;
    return true;
}
bool current_orders(const Observation& o,const Farm& after,const Action& program,int quantity,
                    bool buy_last,Action& action,const Configuration& config) {
    OrderLedger ledger(o,after,config);
    for(int k=0;k<program.n_orders;++k)
        if(early_sale(program,k) && !ledger.append(program.orders[k])) return false;
    if((quantity>0 || (quantity<0 && !buy_last)) &&
       !ledger.append({uint8_t(quantity>0?M_SELL:M_BUY_PRODUCT),WHEAT,std::abs(quantity)})) return false;
    for(int k=0;k<program.n_orders;++k) {
        const auto order=program.orders[k];
        if(order.op==M_NONE || early_sale(program,k) ||
           ((order.op==M_SELL || order.op==M_BUY_PRODUCT) && order.item==WHEAT)) continue;
        if(!ledger.append(order)) return false;
    }
    if(quantity<0 && buy_last && !ledger.append({M_BUY_PRODUCT,WHEAT,-quantity})) return false;
    action.n_orders=ledger.orders.n_orders;
    std::copy_n(ledger.orders.orders,action.n_orders,action.orders); action.finalize();
    return true;
}
// Choose the least wheat stock that executes the fixed worker phase, then keep
// enough free space that additional wheat cannot change its admitted deposits.
// This allows unavoidable DROP loss instead of demanding impossible storage.
bool project_worker_phase(const ResourceSchedule& r,int hour,OrderLedger& ledger,InventoryEvent& previous,
                          const Configuration& config) {
    const int low=previous.minimum_left,high=config.shed_capacity-ledger.total;
    for(int wheat=low;wheat<=high;++wheat) {
        auto candidate=ledger; candidate.stock[WHEAT]=wheat; candidate.total+=wheat;
        int headroom=config.shed_capacity-candidate.total; bool valid=true;
        for(int j=r.shed_transfer_begin[hour];j<r.shed_transfer_begin[hour+1];++j) {
            const auto transfer=r.shed_transfers[j]; const int p=transfer.product;
            if(transfer.quantity<0) {
                if(candidate.stock[p]+transfer.quantity<0) { valid=false; break; }
                candidate.stock[p]+=transfer.quantity; candidate.total+=transfer.quantity;
            } else {
                const int received=std::min(transfer.quantity,config.shed_capacity-candidate.total);
                candidate.stock[p]+=received; candidate.total+=received;
            }
            headroom=std::min(headroom,config.shed_capacity-candidate.total);
        }
        if(!valid) continue;
        previous.minimum_left=wheat;
        previous.maximum_left=std::min(previous.maximum_left,wheat+headroom);
        previous.net_receipt_next=candidate.stock[WHEAT]-wheat;
        candidate.total-=candidate.stock[WHEAT]; candidate.stock[WHEAT]=0;
        ledger=candidate;
        return previous.minimum_left<=previous.maximum_left;
    }
    return false;
}
}
InputContinuation wheat_continuation(const Observation& o,const History& history,const ResourceSchedule& resources,
                                    const Configuration& config) {
    InputContinuation out;
    if(o.day>=29) return out;
    const int day=o.day+1;
    // Low service prevents escape of productive survivors; high service also
    // repeats today's feed. Animals without another usable production are not
    // assigned a universal feed reserve. Both are value scenarios, not bounds.
    for(int c=0;c<100;++c) {
        const auto& t=resources.next_dawn_tiles[c];
        if(!t.has_animal || next_animal_production(t,day)>29) continue;
        const bool mandatory=t.consecutive_dry>=1;
        out.minimum_requirement+=mandatory;
        out.service_requirement+=mandatory || t.consecutive_dry==0;
    }
    int recovery=0,repeated=0;
    // Compare next-dawn purchase with a purchase after the first four phases.
    // Only currently revealed shops and last-day identifiable rival flow enter.
    for(int h=0;h<4;++h) {
        recovery-=demand(o,config,WHEAT,day*24+h);
        for(const auto& sample:history.samples())
            if(sample.step>=0 && sample.step<o.step && sample.step%24==h && sample.identifiable[WHEAT])
                repeated+=std::clamp(sample.lower[WHEAT],-100,100);
    }
    out.scenarios[0]={.25,0,0,out.minimum_requirement};
    out.scenarios[1]={.25,repeated,0,out.minimum_requirement};
    out.scenarios[2]={.25,recovery,0,out.service_requirement};
    out.scenarios[3]={.25,recovery+repeated,0,out.service_requirement};
    return out;
}
ExecutionError InputMarket::orders(const Observation& o,const History& history,const ResourceSchedule& resources,
                                  const Action* program,Action& action,const InputMarketOptions& options,const Configuration& config) {
    const int forecast=options.forecast;
    const auto* continuation=options.continuation;
    action=resources.workers[o.hour]; action.n_orders=0; action.finalize(); diagnostics={}; failure_hour=-1; capacity_shortfall=0;
    failure_product=-1; failure_stock=0; failure_order=0; problem_diagnostics=InventoryProblem();
    if((o.day!=29 && !continuation) || forecast<0 || forecast>2) return ExecutionError::MarketPlan;
    const auto after=worker_phase(o,action,config);
    auto unlimited=config; unlimited.shed_capacity=30000;
    OrderLedger projection(o,after,unlimited); projection.cash=1e9;
    projection.total-=projection.stock[WHEAT]; projection.stock[WHEAT]=0;
    auto& problem=problem_diagnostics; problem.count=resources.hours-o.hour; problem.terminal=o.day==29;
    if(options.prefer_required_purchase)
        problem.preferred_first_quantity=std::min(0,after.shed[WHEAT]-resources.input_remaining_need[o.hour][0]);
    if(!problem.terminal) std::copy_n(continuation->scenarios,4,problem.residual);
    int predicted[24]{};
    if(forecast==2) forecast_sales(o,history,WHEAT,problem.count,predicted,config,true);
    for(int k=0;k<problem.count;++k) {
        const int h=o.hour+k; auto& event=problem.events[k];
        failure_hour=h;
        if(k) {
            if(options.clip_unavoidable_returns) {
                if(!project_worker_phase(resources,h,projection,problem.events[k-1],config)) return ExecutionError::Capacity;
            } else for(int p=0;p<N_ITEMS;++p) if(p!=WHEAT) {
                const int net=resources.receipts[h][p]-resources.pickups[h][p];
                projection.stock[p]+=net; projection.total+=net;
                if(projection.stock[p]<0) { failure_product=p; failure_stock=projection.stock[p]; return ExecutionError::MarketPlan; }
            }
        }
        int non_wheat_orders=0;
        for(bool sales_first:{true,false}) for(int slot=0;slot<program[h].n_orders;++slot) {
            const auto order=program[h].orders[slot];
            if(order.op==M_NONE || ((order.op==M_SELL || order.op==M_BUY_PRODUCT) && order.item==WHEAT) ||
               early_sale(program[h],slot)!=sales_first) continue;
            // Fertilizer can be bought and resold within one hour. Unlike
            // finished goods, moving its sale ahead of the buy is not legal.
            projection.orders.clear();
            if(!projection.append(order)) {
                failure_product=order.item; failure_stock=projection.stock[order.item]; failure_order=order.op;
                return ExecutionError::MarketPlan;
            }
            ++non_wheat_orders;
        }
        event.cash_available=after.money+projection.cash-1e9;
        const int capacity=options.clip_unavoidable_returns?config.shed_capacity:resources.capacity_after_market[h];
        if(projection.total>capacity) {
            capacity_shortfall=projection.total-resources.capacity_after_market[h]; return ExecutionError::Capacity;
        }
        event.maximum_left=std::clamp(capacity-projection.total,0,config.shed_capacity);
        if(h==23 && !problem.terminal) {
            // Other stock is fixed in this conditional component, including
            // unsold finished goods. Admit cargo in engine insertion order.
            int room=config.shed_capacity-projection.total,wheat=0;
            for(int j=0;j<resources.night_deposit_count;++j) {
                const auto deposit=resources.night_deposits[j];
                const int accepted=std::min(room,deposit.quantity); room-=accepted;
                if(deposit.product==WHEAT) wheat+=accepted;
            }
            event.maximum_left=std::min(event.maximum_left,room);
            for(auto& scenario:problem.residual) scenario.usable_receipts=wheat;
        }
        event.minimum_left=resources.input_next_need[h][0];
        event.trade_allowed=non_wheat_orders<config.max_orders;
        event.demand_after=demand(o,config,WHEAT,o.step+k);
        if(h+1<resources.hours) event.net_receipt_next=resources.receipts[h+1][WHEAT]-resources.pickups[h+1][WHEAT];
        event.rival=predicted[k];
        if(forecast==1) for(const auto& sample:history.samples())
            if(sample.step>=0 && sample.step<o.step && sample.step%24==h && sample.identifiable[WHEAT])
                event.rival=std::clamp(sample.lower[WHEAT],-100,100);
        for(int p=0;p<N_PRODUCTS;++p) if(p!=WHEAT) projection.inventory[p]-=demand(o,config,p,o.step+k);
    }
    if(options.protect_purchase_cash) {
        // New optional holdings may use only cash left after the fixed purchase
        // prefixes. Do not borrow committed asset funding against a forecast
        // wheat resale: unobserved within-hour buy/sell cycles can change its
        // execution price despite a small observed rival net flow.
        double spare=problem.events[0].cash_available;
        for(int k=0;k<problem.count;++k) spare=std::min(spare,problem.events[k].cash_available);
        int affordable=0;
        while(affordable<config.shed_capacity &&
              -transact(WHEAT,o.market.inventory[WHEAT],-affordable-1,problem.events[0].rival).own_cash<=spare) ++affordable;
        const int limit=std::max(after.shed[WHEAT]+affordable,resources.input_remaining_need[o.hour][0]);
        problem.events[0].maximum_left=std::min(problem.events[0].maximum_left,limit);
    }
    // Forecast cash and executable current cash are different constraints.
    // Restrict the first inventory choice before solving, so a slightly cheaper
    // predicted rival fill cannot replace all required orders with an error.
    auto& first=problem.events[0];
    int lowest=config.shed_capacity+1,highest=-1;
    Action current=action;
    for(int stock=first.minimum_left;stock<=first.maximum_left;++stock)
        if(current_orders(o,after,program[o.hour],after.shed[WHEAT]-stock,
                          options.buy_after_fixed_orders,current,config)) {
            lowest=std::min(lowest,stock); highest=stock;
        }
    first.minimum_left=lowest; first.maximum_left=highest;
    diagnostics=controller_.solve(problem,after.shed[WHEAT],o.market.inventory[WHEAT]);
    failure_hour=o.hour;
    if(!diagnostics.feasible) return ExecutionError::MarketPlan;
    if(!current_orders(o,after,program[o.hour],diagnostics.quantity,options.buy_after_fixed_orders,action,config))
        return ExecutionError::Funding;
    failure_hour=-1;
    return ExecutionError::None;
}
}
