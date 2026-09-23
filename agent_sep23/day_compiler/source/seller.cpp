#include "seller.hpp"
#include <algorithm>
#include <cmath>

namespace kag::day_compiler {
namespace {
constexpr double IMPOSSIBLE=-1e100;
// Prefix sums give every simultaneous integer sale exact per-unit quotes,
// including the floor where a sold unit stops increasing market inventory.
struct SaleKernel {
    int initial,first_floor=401;
    double prefix[401]{},paired[101]{};
    SaleKernel(int product,int inventory,int stock,int rival,int after):initial(inventory) {
        const int total=stock+rival+after;
        for(int n=0;n<total;++n) {
            const int price=market_price(product,inventory+n);
            prefix[n+1]=prefix[n]+price;
            if(price==1 && first_floor==401) first_floor=n;
        }
        for(int n=0;n<std::min(stock,rival);++n)
            paired[n+1]=paired[n]+market_price(product,inventory+2*n);
    }
    Transaction sell(int own,int rival,int after) const {
        const int both=std::min(own,rival);
        const int effective=std::min(both,(first_floor+1)/2);
        const int offset=2*effective,tail=std::abs(own-rival);
        const double rest=prefix[offset+tail]-prefix[offset];
        Transaction result{initial+offset+std::min(tail,std::max(0,first_floor-offset)),
            paired[both]+(own>rival?rest:0),paired[both]+(rival>own?rest:0)};
        const int start=result.inventory-initial;
        result.rival_cash+=prefix[start+after]-prefix[start];
        result.inventory+=std::min(after,std::max(0,first_floor-start));
        return result;
    }
};
}
double Seller::value(int event,int stock,int inventory) {
    const auto& p=*problem_;
    if(event==p.count) {
        if(p.terminal_stock>=0) return stock==p.terminal_stock?0:IMPOSSIBLE;
        double result=0;
        for(const auto& scenario:p.residual) if(scenario.probability)
            result+=scenario.probability*transact(p.product,inventory+scenario.inventory_change,stock-scenario.requirement,0).own_cash;
        return result;
    }
    if(!result_.complete) return IMPOSSIBLE;
    const uint64_t key=(uint64_t(event)<<40)|(uint64_t(stock)<<32)|uint32_t(inventory);
    const int base=(key*11400714819323198485ull)>>49; int slot=base;
    for(int probe=0;probe<8;++probe) {
        slot=(base+probe)&32767;
        const auto& entry=cache_[slot];
        if(entry.generation!=generation_) break;
        if(entry.key==key) return entry.value;
    }
    ++result_.states;
    const auto& stage=p.events[event];
    const int low=std::max(stage.minimum_sale,stock-stage.maximum_left);
    const int high=stock-std::max(stage.minimum_left,stage.use_next);
    SaleKernel kernel(p.product,inventory,stock,stage.rival,stage.rival_after);
    double best=IMPOSSIBLE;
    for(int x=low;x<=high;++x) {
        if(result_.transitions++>=p.transition_limit) { result_.complete=false; return IMPOSSIBLE; }
        const int next_stock=stock-x-stage.use_next+stage.receipt_next;
        if(next_stock<0 || next_stock>100) continue;
        const auto trade=kernel.sell(x,stage.rival,stage.rival_after);
        const double tail=value(event+1,next_stock,trade.inventory-stage.demand_after);
        if(!result_.complete) return IMPOSSIBLE;
        if(tail<=IMPOSSIBLE/2) continue;
        const double score=trade.own_cash-p.rival_weight*trade.rival_cash-p.holding_per_hour*stage.elapsed*(stock-x)+tail;
        if(event==0) result_.sale_values[x]=score;
        if(event==0 && !x) result_.wait_value=score;
        if(score>best+1e-9) {
            best=score;
            if(event==0) result_.quantity=x;
        }
    }
    cache_[slot]={key,best,generation_}; return best;
}
SaleChoice Seller::solve(const SaleProblem& p,int stock,int inventory) {
    result_={}; result_.sale_values.fill(IMPOSSIBLE); problem_=&p;
    if(p.product<0 || p.product>=N_PRODUCTS || p.count<1 || p.count>SALE_EVENTS || stock<0 || stock>100 || p.terminal_stock>100 ||
       !std::isfinite(p.holding_per_hour) || p.holding_per_hour<0) return result_;
    for(int k=0;k<p.count;++k) {
        const auto& e=p.events[k];
        if(e.rival<0 || e.rival>100 || e.rival_after<0 || e.rival_after>200 ||
           e.receipt_next<0 || e.receipt_next>100 || e.use_next<0 || e.use_next>100 ||
           e.minimum_sale<0 || e.minimum_left<0 || e.maximum_left>100 || e.minimum_left>e.maximum_left || e.elapsed<0) return result_;
    }
    if(++generation_==0) { for(auto& entry:cache_) entry.generation=0; ++generation_; }
    result_.value=value(0,stock,inventory);
    result_.feasible=result_.complete && result_.value>IMPOSSIBLE/2;
    return result_;
}
}
