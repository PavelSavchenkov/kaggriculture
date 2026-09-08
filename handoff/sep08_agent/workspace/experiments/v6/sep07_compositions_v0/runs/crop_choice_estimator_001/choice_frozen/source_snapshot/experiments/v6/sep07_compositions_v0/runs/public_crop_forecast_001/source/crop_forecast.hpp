#pragma once
#include "agents/common/api/agent_api.hpp"
#include <array>
#include <algorithm>

namespace compositions::public_crop_forecast {
using Products=std::array<double,kag::N_PRODUCTS>;
using Days=std::array<Products,30>;
// These are explicit service/harvest hypotheses, not recovered opponent code.
enum class Mode {none=0,latest_known_fert=1,cap_known_fert=2,
                 cap_maintain_active_fert=3,cap_fully_fertilized=4,earliest_known_fert=5};
struct Projection {Days output{};Products held{},future{};int current_crops=0;};

inline Projection crop_output(const kag::agent::AgentObservation&o,Mode mode){
    Projection result;if(mode==Mode::none)return result;
    for(const auto&row:o.opponent().tiles)for(const auto&t:row){
        if(t.kind!=kag::T_PLANT)continue;++result.current_crops;
        const auto&c=kag::CROPS[t.what];const int item=t.what,placed=t.planted_day;
        const bool maintain=mode==Mode::cap_fully_fertilized ||
            (mode==Mode::cap_maintain_active_fert && t.fertilized_until_day>=o.day);
        auto fertilized=[&](int day){return t.fertilized_until_day>=day || maintain;};
        int held=t.yield_units;
        if(c.ongoing){
            // Sell currently visible stored yield today. Future production is
            // limited to this crop's four dated events; never infer replanting.
            if(held>0 && o.day-placed>=c.first_yield_day){result.output[o.day][item]+=held;result.held[item]+=held;}
            for(int day=o.day+1;day<30;++day){
                const int since=day-placed-c.first_yield_day;
                if(since<0 || since%c.interval || since/c.interval>=c.max_yield)continue;
                const int quantity=fertilized(day-1)?2:1;
                result.output[day][item]+=quantity;result.future[item]+=quantity;
            }
        }else{
            const int last=std::min(29,std::max(o.day,placed+c.max_yield_day));
            int added=0;
            for(int day=o.day;day<=last;++day){
                const int age=day-placed;
                // Daily productive water is assumed. Already observed water
                // has already changed held yield and cannot be credited twice.
                if((day>o.day || !t.watered_today) && age>=(c.max_yield_day+1)/2 && age<=c.max_yield_day){
                    const int before=held;held=std::min(c.max_yield,held+(fertilized(day)?2:1));added+=held-before;
                }
                const bool ready=age>=c.first_yield_day;
                const bool harvest=day==last || mode==Mode::earliest_known_fert ||
                    (mode!=Mode::latest_known_fert && held>=c.max_yield);
                if(ready && harvest){result.output[day][item]+=held;result.held[item]+=held-added;result.future[item]+=added;break;}
            }
        }
    }return result;
}

// Separate diagnostic disposition assumption: all visible animals remain fed.
// Netting wheat may imply buys despite unknown private stocks; callers must
// label this control separately from predicted biological crop production.
inline void net_visible_herd_feed(const kag::agent::AgentObservation&o,Days&net){
    int animals=0,unfed=0;for(const auto&row:o.opponent().tiles)for(const auto&t:row)if(t.has_animal){++animals;unfed+=!t.fed_today;}
    net[o.day][kag::WHEAT]-=unfed;for(int day=o.day+1;day<30;++day)net[day][kag::WHEAT]-=animals;
}
}
