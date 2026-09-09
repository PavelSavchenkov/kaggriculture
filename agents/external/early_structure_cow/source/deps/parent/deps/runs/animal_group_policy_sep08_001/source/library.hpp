#pragma once
#include "../../../include/guarded_day.hpp"
#include "../../late_portfolio_001/source/model.hpp"
namespace catalog_early_structure_cow_parent_compositions::animal_groups_policy {
struct Course {
    std::vector<GuardedDay> days;
    FarmFlowPlan flow;
    std::array<double,30> fixed{};
};
struct Family {
    int first=0,count=0;
    std::array<std::array<Course,2>,5> choices;
};
using Library=std::array<Family,2>;
const Library& library();
inline void prepare(Course& course){
    for(const auto& day:course.days){
        int hired=0;const int d=day.plan.day;
        for(const auto& a:day.plan.actions)for(int s=0;s<a.n_orders;++s){
            const auto& m=a.orders[s];const int n=std::max(0,int(m.n));
            if(m.op==kag::M_SELL && m.item<kag::N_PRODUCTS)course.flow.sales[d][m.item]+=n;
            else if(m.op==kag::M_BUY_PRODUCT)course.flow.buys[d][m.item]+=n;
            else if(m.op==kag::M_BUY_SEED)course.fixed[d]+=n*kag::CROPS[m.item].seed;
            else if(m.op==kag::M_BUY_ANIMAL)course.fixed[d]+=n*kag::ANIMALS[m.item-kag::GOOSE].cost;
            else if(m.op==kag::M_HIRE)course.fixed[d]+=kag::fib(hired++);
            else if(m.op==kag::M_BUY_LAND)std::abort();
        }
    }
}
}
