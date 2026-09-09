#include "library.hpp"
namespace catalog_early_structure_cow_parent_compositions::cow_service_retained {
namespace {
#include "data.inc"
}
const animal_groups_policy::Library& library(){
    static const auto result=[] {
        auto result=animal_groups_policy::library();const int* p=data;
        for(int leaf=0;leaf<2;++leaf){
            auto& course=result[0].choices[2][leaf];course={};course.days.reserve(15);
            for(int day=15;day<30;++day){
                GuardedDay g{};g.plan.day=*p++;g.quadrants=*p++;
                for(int cell=0;cell<100;++cell){g.check[cell]=*p++;for(auto& x:g.tiles[cell])x=*p++;}
                for(auto& x:g.shed)x=*p++;for(auto& x:g.seeds)x=*p++;
                for(auto& a:g.plan.actions){
                    a.n_units=*p++;a.n_orders=*p++;
                    for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
                    for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
                    a.finalize();
                }
                if(g.plan.day!=day)std::abort();course.days.push_back(g);
            }
            animal_groups_policy::prepare(course);
        }
        if(p!=data+std::size(data))std::abort();return result;
    }();return result;
}
}
