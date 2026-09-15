#pragma once
#include "policy.hpp"
#include "placement.hpp"
#include "verify.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <numeric>

namespace contract_benchmark {
// Counterfactual layout diagnostic: preserve the dawn tile multiset and event
// identities, place oldest product generations first with the actual placement
// policy. This is not a legal within-game movement of established products.
inline kag::agents::day_policy_contract::DayInput repack(const kag::agents::day_policy_contract::DayInput& original) {
    using namespace kag;using namespace kag::agents::day_policy_contract;
    auto out=original;std::fill_n(out.events,100,0);
    for(int c=0;c<100;++c){out.grid[c]=Tile{};out.grid[c].max_lifespan_step=INT_MAX;if(original.grid[c].kind==T_LOCKED)out.grid[c].kind=T_LOCKED;}
    int order[100];std::iota(order,order+100,0);
    auto product=[&](int c){return original.grid[c].has_animal || original.grid[c].kind==T_PLANT;};
    std::stable_sort(order,order+100,[&](int a,int b){
        if(product(a)!=product(b))return product(a)>product(b);
        if(!product(a))return a<b;
        const auto& x=original.grid[a];const auto& y=original.grid[b];
        return x.planted_day!=y.planted_day?x.planted_day<y.planted_day:a<b;
    });
    bool used[100]{};
    for(int start=0;start<100 && product(order[start]);) {
        const int age=original.grid[order[start]].planted_day;
        int end=start;while(end<100 && product(order[end]) && original.grid[order[end]].planted_day==age)++end;
        auto sim=detail::initial_state(out);auto obs=agent::runtime::make_observation(sim,0);
        DayPlan plan;plan.relocate_new=true;
        for(int k=start;k<end;++k) {
            const auto& t=original.grid[order[k]];auto& j=plan.jobs[plan.count++];j.new_site=true;j.count=1;
            j.steps[0]={uint8_t(t.has_animal?OP_PLACE:OP_PLANT),t.what,1};
        }
        place_day(obs,plan);
        for(int k=start;k<end;++k){const int target=plan.jobs[k-start].tile;if(target<0)std::abort();out.grid[target]=original.grid[order[k]];out.events[target]=original.events[order[k]];used[target]=true;}
        start=end;
    }
    for(int c:order) {
        if(product(c) || original.grid[c].kind==T_LOCKED)continue;
        int target=0;while(target<100 && (used[target] || out.grid[target].kind==T_LOCKED))++target;
        if(target==100)std::abort();used[target]=true;out.grid[target]=original.grid[c];out.events[target]=original.events[c];
    }
    return out;
}
}
