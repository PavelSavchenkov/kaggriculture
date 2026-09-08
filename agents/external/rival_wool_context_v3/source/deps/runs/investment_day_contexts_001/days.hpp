#pragma once
#include "../investment_day_library_001/days.hpp"
#include "../investment_day_library_002/days.hpp"
namespace catalog_rival_wool_context_v3_compositions::investment_context_days {
using Entry=investment_days::Entry;
inline std::vector<Entry> entries() {
    auto result=investment_days::entries();
    for(auto& entry:investment_days_second::entries())result.push_back({entry.id+100,std::move(entry.day)});
    return result;
}
inline std::vector<GuardedDay> select(const std::vector<int>& ids) {
    auto all=entries();std::vector<GuardedDay> result;
    for(int id:ids) {
        auto found=std::find_if(all.begin(),all.end(),[&](const auto& entry){return entry.id==id;});
        if(found==all.end())std::abort();result.push_back(found->day);
    }
    return result;
}
}
