#include "rolling.hpp"
#include <algorithm>
#include <cmath>

namespace kag::day_compiler {
SaleTimeline rolling_sales(const Observation& o,const History& history,const ResourceSchedule& resources,
                           int product,const Configuration& config,const RollingSales& options) {
    SaleTimeline out; auto& problem=out.problem;
    if(product<CARROT || product>WOOL || options.recoveries<1 || options.recoveries>6 ||
       options.holding_per_hour<0 || !std::isfinite(options.holding_per_hour)) return out;
    const int last=config.episode_steps-2;
    if(o.step>last || o.hour<0 || o.hour>=resources.hours) return out;
    const auto night=settle_night(resources,config);
    if(!night.valid) return out;
    problem.product=product; problem.holding_per_hour=options.holding_per_hour;
    out.steps[problem.count++]=o.step;
    int recoveries=0;
    for(int step=o.step+1;step<=last;++step) {
        const bool recovery=demand(o,config,product,step-1)>0;
        // Keep each remaining current-day phase. Besides receipts, this retains
        // funding deadlines and the market immediately before a worker deposit.
        const bool planned=options.planned_receipts && step<o.day*24+resources.hours;
        const bool night_event=options.planned_receipts && resources.hours==24 && step==(o.day+1)*24 && night.receipts[product]>0;
        if(recovery || planned || night_event || step==last) {
            if(problem.count==SALE_EVENTS) return out;
            out.steps[problem.count++]=step;
        }
        if(recovery && ++recoveries==options.recoveries) break;
    }
    out.game_end=out.steps[problem.count-1]==last;
    auto features=flow_features(o,history,product,1,config);
    const auto long_features=flow_features(o,history,product,24,config);
    auto cumulative=[&](int horizon) {
        if(horizon<=0) return 0.0;
        auto x=horizon>=24?long_features:features; x[2]=std::min(horizon,24);
        return predict_flow(product,x)*std::max(1.0,horizon/24.0);
    };
    double previous=0;
    for(int k=0;k<problem.count;++k) {
        const int step=out.steps[k],h=step-o.day*24; auto& event=problem.events[k];
        const double at=std::max(previous,cumulative(step-o.step));
        const double after=std::max(at,cumulative(step-o.step+1));
        event.rival=int(std::lround(after)-std::lround(at));
        if(event.rival>100) return out;
        if(h<resources.hours && (options.planned_receipts || k==0)) {
            event.minimum_left=resources.reserve_after_market[h][product];
            int other=0; for(int p=0;p<N_ITEMS;++p) if(p!=product) other+=resources.reserve_after_market[h][p];
            event.maximum_left=std::max(0,resources.capacity_after_market[h]-other);
            if(h==23 && options.planned_receipts) {
                event.maximum_left=std::min(event.maximum_left,std::max(0,night.capacity_before-std::max(other,night.locked_stock)));
            }
        }
        if(k+1<problem.count) {
            const int next=out.steps[k+1];
            const double future=std::max(after,cumulative(next-o.step));
            event.rival_after=int(std::lround(future)-std::lround(after));
            if(event.rival_after>200) return out;
            previous=future; event.elapsed=next-step;
            for(int t=step;t<next;++t) event.demand_after+=demand(o,config,product,t);
            if(options.planned_receipts && next<o.day*24+resources.hours) {
                // minimum_left already enforces the exact worker-prefix need.
                // Net the phase here: an earlier worker deposit can supply a
                // later worker pickup, so requiring all pickups in advance is wrong.
                const int net=resources.receipts[next%24][product]+resources.purchases[next%24][product]-resources.pickups[next%24][product];
                event.receipt_next=std::max(0,net); event.use_next=std::max(0,-net);
            }
            if(options.planned_receipts && resources.hours==24 && next==(o.day+1)*24)
                event.receipt_next=night.receipts[product];
        }
    }
    out.valid=true; return out;
}
}
