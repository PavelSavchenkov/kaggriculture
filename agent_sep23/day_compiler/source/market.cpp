#include "market.hpp"
#include "allocation.hpp"
#include "forecast.hpp"
#include "rolling.hpp"
#include <algorithm>
#include <cmath>

namespace kag::day_compiler {
namespace {
SaleProblem sale_problem(const Observation& o,const History& history,const ResourceSchedule& resources,
                         int product,MarketMode mode,const Configuration& config) {
    if(uses_rolling_sales(mode) && product>=CARROT && product<=WOOL) {
        RollingSales options; options.recoveries=(mode==MarketMode::SellerRollingH3 || mode==MarketMode::SellerRollingStockH3)?3:6;
        options.planned_receipts=uses_planned_sale_receipts(mode);
        const auto timeline=rolling_sales(o,history,resources,product,config,options);
        return timeline.valid?timeline.problem:SaleProblem{};
    }
    SaleProblem problem; problem.product=product; problem.count=resources.hours-o.hour;
    int predicted[24]{};
    if(uses_model(mode)) forecast_sales(o,history,product,problem.count,predicted,config,uses_terminal_balance(mode));
    for(int k=0;k<problem.count;++k) {
        const int h=o.hour+k,step=o.step+k; auto& event=problem.events[k];
        event.rival=predicted[k];
        event.minimum_left=resources.reserve_after_market[h][product];
        int reserved_other=0;
        for(int q=0;q<N_ITEMS;++q) if(q!=product) reserved_other+=resources.reserve_after_market[h][q];
        event.maximum_left=std::max(0,resources.capacity_after_market[h]-reserved_other);
        event.demand_after=demand(o,config,product,step);
        if(h+1<resources.hours) {
            event.receipt_next=resources.receipts[h+1][product]+resources.purchases[h+1][product];
            event.use_next=resources.pickups[h+1][product];
        }
        if(mode==MarketMode::SellerRecentFlow) for(const auto& sample:history.samples())
            if(sample.step>=0 && sample.step<o.step && sample.step%24==step%24 && sample.identifiable[product])
                event.rival=std::clamp(sample.lower[product],0,100);
    }
    return problem;
}
}
ExecutionError MarketPlanner::orders(const Observation& o,const History& history,const Farm& workers,
                                    const ResourceSchedule& resources,MarketMode mode,Action& action,const Configuration& config) {
    inventory_diagnostics={}; failure.stage=0; failure.count=0;
    if(mode==MarketMode::Liquidate || (o.day!=29 && !uses_rolling_sales(mode)))
        return liquidation_orders(o,workers,resources,action,config);
    if(o.day==29 && uses_rolling_sales(mode)) mode=MarketMode::SellerTerminalJoint;
    const int hour=o.hour;
    int capacity=resources.capacity_after_market[hour];
    if(uses_planned_sale_receipts(mode) && hour==23) {
        const auto night=settle_night(resources,config);
        if(!night.valid) return ExecutionError::MarketPlan;
        capacity=std::min(capacity,night.capacity_before);
    }
    const bool control_wheat=controls_wheat(mode) && !resources.preserve_input_orders;
    auto flexible=[&](int p) { return resources.flexible_inputs && (p==WHEAT || p==FERTILIZER); };
    if(resources.flexible_inputs && resources.preserve_input_orders) return ExecutionError::MarketPlan;
    OrderLedger ledger(o,workers,config);
    int future_buys[N_ITEMS]; std::copy_n(resources.purchases[hour],N_ITEMS,future_buys);
    int future_sales[N_PRODUCTS]; std::copy_n(resources.fixed_sales[hour],N_PRODUCTS,future_sales);
    auto reserve=[&](int p) {
        if(control_wheat && p==WHEAT) return resources.input_next_need[hour][0];
        if(flexible(p)) return resources.input_remaining_need[hour][p==FERTILIZER];
        return std::max(0,resources.reserve_after_market[hour][p]-future_buys[p]+future_sales[p]);
    };
    int fixed_orders_remaining=0;
    for(int k=0;k<resources.workers[hour].n_orders;++k) {
        const auto x=resources.workers[hour].orders[k];
        if(x.op==M_NONE || (x.op==M_BUY_PRODUCT && flexible(x.item)) ||
           (control_wheat && x.item==WHEAT && (x.op==M_SELL || x.op==M_BUY_PRODUCT))) continue;
        ++fixed_orders_remaining;
    }
    auto sale=[&](int product,int quantity) {
        if(uses_rolling_sales(mode) && ledger.extend_finished_sale(product,quantity)) return true;
        return ledger.append({M_SELL,uint8_t(product),quantity});
    };
    int input_orders_remaining=0;
    for(int p:{WHEAT,FERTILIZER}) if(flexible(p) && !(control_wheat && p==WHEAT)) {
        future_buys[p]=std::max(0,resources.input_buy_need[hour][p==FERTILIZER]-ledger.stock[p]);
        input_orders_remaining+=future_buys[p]>0;
    }
    auto fund=[&](Order pending) {
        int required_space=0;
        if(uses_rolling_sales(mode) && (resources.preserve_input_orders || resources.planned_input_purchases) &&
           ledger.orders.n_orders+fixed_orders_remaining+input_orders_remaining+1==config.max_orders) {
            int final_total=ledger.total;
            for(int p=0;p<N_ITEMS;++p) final_total+=future_buys[p]-(p<N_PRODUCTS?future_sales[p]:0);
            required_space=std::max(0,final_total-capacity);
        }
        if(mode>=MarketMode::SellerTerminalWheatSpace) {
            int best=-1,quantity=0; double regret=1e100;
            for(int p=0;p<N_PRODUCTS;++p) {
                if(resources.preserve_input_orders && (p==WHEAT || p==FERTILIZER)) continue;
                const int available=std::max(0,ledger.stock[p]-reserve(p));
                auto enables=[&](int q) {
                    auto trial=ledger;
                    return trial.append({M_SELL,uint8_t(p),q}) && trial.append(pending);
                };
                if(!available || available<required_space || !enables(available)) continue;
                int low=1,high=available;
                while(low<high) { const int q=(low+high)/2; if(enables(q)) high=q; else low=q+1; }
                low=std::max(low,required_space);
                const auto problem=sale_problem(o,history,resources,p,mode,config);
                const auto choice=seller_.solve(problem,ledger.stock[p],ledger.inventory[p]);
                if(!choice.feasible || choice.sale_values[low]<-1e99) continue;
                const double loss=choice.value-choice.sale_values[low];
                if(loss<regret) { best=p; quantity=low; regret=loss; }
            }
            if(best>=0) return sale(best,quantity);
        }
        int best=-1; double value=-1;
        for(int p=0;p<N_PRODUCTS;++p) {
            if(resources.preserve_input_orders && (p==WHEAT || p==FERTILIZER)) continue;
            const int q=std::max(0,ledger.stock[p]-reserve(p));
            const double cash=transact(p,ledger.inventory[p],q,0).own_cash;
            if(q>=std::max(1,required_space) && cash>value) { best=p; value=cash; }
        }
        return best>=0 && sale(best,ledger.stock[best]-reserve(best));
    };
    const auto& fixed=resources.workers[hour];
    for(int p:{WHEAT,FERTILIZER}) if(flexible(p) && !(control_wheat && p==WHEAT)) {
        const int quantity=std::max(0,resources.input_buy_need[hour][p==FERTILIZER]-ledger.stock[p]);
        if(!quantity) continue;
        const Order order{M_BUY_PRODUCT,uint8_t(p),quantity};
        while(!ledger.append(order)) {
            if(ledger.orders.n_orders>=config.max_orders) return ExecutionError::MarketSlots;
            if(!fund(order)) return ExecutionError::Funding;
        }
        future_buys[p]-=quantity; --input_orders_remaining;
    }
    for(int k=0;k<fixed.n_orders;++k) {
        const auto order=fixed.orders[k];
        if(control_wheat && (order.op==M_BUY_PRODUCT || order.op==M_SELL) && order.item==WHEAT) continue;
        if(order.op==M_BUY_PRODUCT && flexible(order.item)) continue;
        while(!ledger.append(order)) {
            if(ledger.orders.n_orders>=config.max_orders) return ExecutionError::MarketSlots;
            if(!fund(order)) return ExecutionError::Funding;
        }
        if(order.op!=M_NONE) --fixed_orders_remaining;
        if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL) future_buys[order.item]-=order.n;
        if(order.op==M_SELL) future_sales[order.item]-=order.n;
    }
    struct Proposal { int product,quantity; double regret; } proposals[N_PRODUCTS];
    SaleCurve curves[N_PRODUCTS];
    int count=0;
    for(int p=0;p<N_PRODUCTS;++p) {
        if(resources.preserve_input_orders && (p==WHEAT || p==FERTILIZER)) continue;
        if(control_wheat && p==WHEAT) {
            InventoryProblem problem; problem.count=resources.hours-hour; problem.terminal=true;
            problem.rival_weight=mode==MarketMode::SellerTerminalInventoryFlow || mode==MarketMode::SellerTerminalInventoryRecent;
            int predicted[24]{};
            if(mode>=MarketMode::SellerTerminalInventoryFlow) forecast_sales(o,history,WHEAT,problem.count,predicted,config,true);
            double cash=ledger.cash; int hires=ledger.hires;
            int input_inventory=ledger.inventory[FERTILIZER],quadrants=ledger.quadrants;
            for(int k=0;k<problem.count;++k) {
                const int h=hour+k; auto& event=problem.events[k];
                event.rival=predicted[k];
                if(mode==MarketMode::SellerTerminalInventoryRecent) {
                    event.rival=0;
                    for(const auto& sample:history.samples())
                        if(sample.step>=0 && sample.step<o.step && sample.step%24==h && sample.identifiable[WHEAT])
                            event.rival=std::clamp(sample.lower[WHEAT],-100,100);
                }
                int slots=0;
                for(int n=0;n<resources.workers[h].n_orders;++n) {
                    const auto order=resources.workers[h].orders[n];
                    if(order.op==M_NONE || ((order.op==M_BUY_PRODUCT || order.op==M_SELL) && order.item==WHEAT)) continue;
                    ++slots; if(!k) continue;
                    if(order.op==M_HIRE) cash-=config.hire_mult*fib(hires++);
                    else if(order.op==M_BUY_SEED) cash-=CROPS[order.item].seed*order.n;
                    else if(order.op==M_BUY_ANIMAL) cash-=ANIMALS[order.item-GOOSE].cost*order.n;
                    else if(order.op==M_BUY_PRODUCT) {
                        const auto trade=transact(order.item,input_inventory,-order.n,0);
                        cash+=trade.own_cash; input_inventory=trade.inventory;
                    } else if(order.op==M_BUY_LAND) cash-=LAND_PRICES[quadrants++-1];
                }
                event.cash_available=cash;
                event.trade_allowed=slots<config.max_orders;
                event.minimum_left=resources.input_next_need[h][0];
                int reserved_other=0;
                for(int q=0;q<N_ITEMS;++q) if(q!=WHEAT) reserved_other+=resources.reserve_after_market[h][q];
                event.maximum_left=std::max(0,resources.capacity_after_market[h]-reserved_other);
                event.demand_after=demand(o,config,WHEAT,o.step+k);
                if(h+1<resources.hours) event.net_receipt_next=resources.receipts[h+1][WHEAT]-resources.pickups[h+1][WHEAT];
            }
            const auto values=inventory_.curve(problem,ledger.stock[WHEAT],ledger.inventory[WHEAT]);
            inventory_diagnostics=values;
            if(!values.complete) return ExecutionError::MarketPlan;
            auto& curve=curves[count]; curve.product=WHEAT; curve.stock=ledger.stock[WHEAT];
            curve.maximum_buy=100-curve.stock;
            for(int left=0;left<=100;++left) {
                const int quantity=curve.stock-left;
                if(quantity<0) curve.buy_value[-quantity]=values.value[left]; else curve.value[quantity]=values.value[left];
            }
            proposals[count++]={WHEAT,0,0};
            continue;
        }
        const int surplus=ledger.stock[p]-reserve(p); if(surplus<=0) continue;
        if(p==FERTILIZER || (p==WHEAT && !uses_wheat_dp(mode))) {
            if(uses_joint_sales(mode)) {
                if(ledger.orders.n_orders<config.max_orders && !ledger.append({M_SELL,uint8_t(p),surplus})) return ExecutionError::Funding;
            } else proposals[count++]={p,surplus,1e50};
            continue;
        }
        const auto problem=sale_problem(o,history,resources,p,mode,config);
        const auto choice=seller_.solve(problem,ledger.stock[p],ledger.inventory[p]);
        if(!choice.feasible) { failure.stage=1; failure.product=p; return ExecutionError::MarketPlan; }
        curves[count].product=p; curves[count].stock=ledger.stock[p]; curves[count].value=choice.sale_values;
        if(uses_rolling_sales(mode)) for(int k=0;k<ledger.orders.n_orders;++k)
            curves[count].existing_sale_order|=ledger.orders.orders[k].op==M_SELL && ledger.orders.orders[k].item==p;
        proposals[count++]={p,choice.quantity,choice.value-choice.wait_value};
    }
    // Join product decisions on the actual shared stock. Reserve next DROP space
    // now: sales after that worker phase cannot recover discarded output.
    int expected_total=ledger.total;
    for(int k=0;k<count;++k) expected_total-=proposals[k].quantity;
    if(uses_joint_sales(mode)) {
        int other=ledger.total; for(int k=0;k<count;++k) other-=curves[k].stock;
        const auto joined=allocate_sales(curves,count,other,capacity,config.max_orders-ledger.orders.n_orders);
        if(!joined.feasible) {
            failure.stage=2; failure.capacity=capacity; failure.total=ledger.total;
            failure.slots=config.max_orders-ledger.orders.n_orders; failure.count=count; failure.orders=ledger.orders;
            for(int k=0;k<count;++k) {
                failure.products[k]=curves[k].product; failure.stock[k]=curves[k].stock;
                failure.reusable[k]=curves[k].existing_sale_order; failure.minimum_sale[k]=101;
                for(int q=0;q<=curves[k].stock;++q) if(curves[k].value[q]>-1e90) { failure.minimum_sale[k]=q; break; }
            }
            return ExecutionError::MarketPlan;
        }
        for(int k=0;k<count;++k) {
            proposals[k].quantity=joined.quantity[proposals[k].product];
            const int q=proposals[k].quantity;
            proposals[k].regret=(q<0?curves[k].buy_value[-q]:curves[k].value[q])-curves[k].value[0];
        }
    } else if(expected_total>resources.capacity_after_market[hour]) {
        std::sort(proposals,proposals+count,[](const auto& a,const auto& b) { return a.regret>b.regret; });
        for(int k=0;k<count && expected_total>resources.capacity_after_market[hour];++k) {
            auto& x=proposals[k];
            const int extra=std::min(expected_total-resources.capacity_after_market[hour],ledger.stock[x.product]-reserve(x.product)-x.quantity);
            x.quantity+=extra; expected_total-=extra;
        }
    }
    std::sort(proposals,proposals+count,[](const auto& a,const auto& b) {
        if((a.quantity<0)!=(b.quantity<0)) return a.quantity>=0;
        return a.regret>b.regret;
    });
    for(int k=0;k<count;++k) if(proposals[k].quantity) {
        const int q=proposals[k].quantity,p=proposals[k].product;
        if(q>0 && uses_rolling_sales(mode) && ledger.extend_finished_sale(p,q)) continue;
        if(ledger.orders.n_orders==config.max_orders) break;
        if(!ledger.append({uint8_t(q>0?M_SELL:M_BUY_PRODUCT),uint8_t(p),std::abs(q)})) return ExecutionError::Funding;
    }
    if(ledger.total>capacity) return ExecutionError::Capacity;
    for(int p=0;p<N_ITEMS;++p)
        if(ledger.stock[p]<((control_wheat && p==WHEAT)?resources.input_next_need[hour][0]:flexible(p)?resources.input_buy_need[hour][p==FERTILIZER]:resources.reserve_after_market[hour][p]))
            return ExecutionError::Funding;
    action=fixed; action.n_orders=ledger.orders.n_orders;
    std::copy_n(ledger.orders.orders,action.n_orders,action.orders); action.finalize();
    return ExecutionError::None;
}
}
