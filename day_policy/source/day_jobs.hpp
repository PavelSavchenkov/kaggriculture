#pragma once
#include "verify.hpp"

namespace kag::agents::day_policy_contract {
inline DayPlan compile_day_jobs(const DayInput& in, int hires, int variant, bool split) {
    DayPlan plan; plan.day = detail::CALENDAR; plan.hires = hires;
    plan.relocate_new = true; plan.trade = false; plan.buy_land = in.land_hour >= 0; plan.land_hour = in.land_hour;
    std::copy_n(in.buy_seeds, N_CROPS, plan.buy_seeds);
    for (int h = 0; h < 24; ++h) plan.buy_items[WHEAT] += in.buy_wheat[h];
    for (int a = 0; a < 3; ++a) plan.buy_items[GOOSE+a] = in.buy_animals[a];
    auto push = [](Job& job, int op, int arg = 0) { job.steps[job.count++] = {uint8_t(op),uint8_t(arg),1}; };
    for (int c = 0; c < 100; ++c) {
        auto e = in.events[c]; if (!e) continue;
        const auto& t = in.grid[c]; Job job; job.tile = job.key = c;
        const bool independent = t.has_animal || (t.kind == T_PLANT && CROPS[t.what].ongoing && !(e & Clear) && t.max_lifespan_step > 23);
        if (independent) {
            for (auto [flag,op] : {std::pair{int(CollectFertilizer),int(OP_COLLECT_FERTILIZER)}, {int(Harvest),int(OP_HARVEST)}})
                if (e & flag) { Job separate = job; push(separate,op); plan.jobs[plan.count++] = separate; e &= ~flag; }
        }
        if ((variant & 1) && independent && (e & Water)) { Job separate=job; push(separate,OP_WATER); plan.jobs[plan.count++]=separate; e &= ~Water; }
        if (e & Fertilize) push(job,OP_FERTILIZE);
        if (e & Water) push(job,OP_WATER);
        if (e & Feed) push(job,OP_FEED);
        if (e & Care) push(job,OP_CARE);
        if (e & CollectFertilizer) push(job,OP_COLLECT_FERTILIZER);
        if (split && t.kind==T_PLANT && !CROPS[t.what].ongoing &&
            (e & Water) && (e & Harvest) && in.returns[23][t.what]>0) {
            const int prefix=plan.count;
            plan.jobs[plan.count++]=job;
            job=Job{};job.tile=job.key=c;job.service_predecessor=prefix;
            job.prior_service=e&(Water|Fertilize);
        }
        if (e & Harvest) push(job,OP_HARVEST);
        if ((e & Clear) && !(t.kind == T_PLANT && !CROPS[t.what].ongoing && (e & Harvest))) push(job,OP_DIG);
        if (job.count) plan.jobs[plan.count++] = job;
    }
    for (int n = 0; n < in.establish_count; ++n) {
        const auto product = in.establish[n]; Job job; job.new_site = true; job.key = -100-n;
        push(job,is_crop(product.product) ? OP_PLANT : OP_PLACE,product.product);
        if (product.events & Fertilize) push(job,OP_FERTILIZE);
        if (product.events & Water) push(job,OP_WATER);
        if (product.events & Feed) push(job,OP_FEED);
        if (product.events & Care) push(job,OP_CARE);
        plan.jobs[plan.count++] = job;
    }
    return plan;
}

}
