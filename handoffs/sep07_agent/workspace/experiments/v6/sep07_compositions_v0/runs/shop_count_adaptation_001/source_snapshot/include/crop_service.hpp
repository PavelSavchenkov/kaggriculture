#pragma once
#include "biology.hpp"

namespace compositions {
inline int crop_last_age(int item) {
    const auto& c=kag::CROPS[item];
    return c.ongoing?c.first_yield_day+(c.max_yield-1)*c.interval:c.max_yield_day;
}

// Earliest harvest with the same total output as the final productive age.
// This also handles crops whose unfertilized maximum is below the yield cap.
inline int earliest_full_crop_age(int item,bool fertilize) {
    if(!kag::is_crop(item))std::abort();
    const auto& crop=kag::CROPS[item];const int last=crop_last_age(item);
    if(crop.ongoing)return last;
    auto output=[&](int age) {
        Cohort c{uint8_t(item),1,0,age+1};auto service=productive_service(c,fertilize);
        service.harvest=uint32_t{1}<<age;
        const auto b=biology(c,service);int sum=0;
        for(const auto& d:b.days)sum+=d.output[item];return sum;
    };
    const int target=output(last);
    for(int age=crop.first_yield_day;age<=last;++age)if(output(age)==target)return age;
    std::abort();
}

// Local deletion search under exact dated biology, without a route or funding
// claim. Preserve every output day and plant occupancy; do not assume service
// is mandatory merely because the starting productive policy requests it.
inline Service prune_crop_service(const Cohort& c,Service service) {
    if(!kag::is_crop(c.item))std::abort();
    const auto baseline=biology(c,service);
    auto equal=[&](const Service& proposal) {
        const auto b=biology(c,proposal);
        for(int day=0;day<30;++day) {
            if(b.days[day].active!=baseline.days[day].active)return false;
            for(int i=0;i<kag::N_PRODUCTS;++i)if(b.days[day].output[i]!=baseline.days[day].output[i])return false;
        }
        return true;
    };
    for(int kind=0;kind<2;++kind)for(int day=c.end_day-1;day>=c.start_day;--day) {
        auto candidate=service;auto& mask=kind==0?candidate.fertilize:candidate.water;
        if(!on(mask,day))continue;mask&=~(uint32_t{1}<<day);
        if(equal(candidate))service=candidate;
    }
    return service;
}
}
