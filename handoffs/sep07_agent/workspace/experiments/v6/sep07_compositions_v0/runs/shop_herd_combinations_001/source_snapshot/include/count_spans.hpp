#pragma once
#include "biology.hpp"
#include "crop_service.hpp"
#include "composition.hpp"
#include <algorithm>
#include <vector>

namespace compositions {
// Maintain a requested count throughout [start_day,end_day). Crop rotation
// age is a lifecycle-module alternative, never a worker action or tile choice.
// -1 uses the final productive age; -2 the earliest equal-output harvest.
// Even an immature final rotation is retained
// as requested intent and reported; an economic search may then shorten it.
struct CountSpan {
    Cohort occupancy;
    int crop_harvest_age=-1;
    bool fertilize=false;
    bool prune_service=false;
};

struct SpanExpansion {
    std::vector<Life> lives;
    int requested_count_days=0,immature_count_days=0;
};

inline SpanExpansion expand_count_spans(std::span<const CountSpan> spans) {
    SpanExpansion result;
    for(const auto& span:spans) {
        const auto& count=span.occupancy;
        if(count.count<0 || count.start_day<0 || count.start_day>=count.end_day || count.end_day>30 ||
            !(kag::is_crop(count.item) || kag::is_animal(count.item)))std::abort();
        if(!count.count)continue;
        result.requested_count_days+=count.count*(count.end_day-count.start_day);
        const bool animal=kag::is_animal(count.item);
        const auto crop=animal?kag::CropDef{}:kag::CROPS[count.item];
        int duration=count.end_day-count.start_day;
        if(!animal) {
            const int last=crop_last_age(count.item);
            const int harvest=span.crop_harvest_age==-2?earliest_full_crop_age(count.item,span.fertilize):span.crop_harvest_age<0?last:span.crop_harvest_age;
            if(harvest<crop.first_yield_day || harvest>last)std::abort();
            duration=harvest+1;
        }
        for(int start=count.start_day;start<count.end_day;start+=duration) {
            const int end=std::min(count.end_day,start+duration);
            Cohort rotation{count.item,count.count,start,end};
            auto service=productive_service(rotation,span.fertilize);
            if(!animal && !crop.ongoing)service.harvest=uint32_t{1}<<(end-1);
            if(!animal && span.prune_service)service=prune_crop_service(rotation,service);
            if(!animal && end-start<=crop.first_yield_day)
                result.immature_count_days+=count.count*(end-start);
            for(int n=0;n<count.count;++n)
                result.lives.push_back({count.item,start*24,std::min(719,end*24),0,0,
                    service.fertilize,service.water,service.feed,service.care,service.collect_fertilizer,service.harvest});
        }
    }
    std::stable_sort(result.lives.begin(),result.lives.end(),[](const Life& a,const Life& b){return a.start<b.start;});
    return result;
}
}
