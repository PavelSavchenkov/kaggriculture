#include "library.hpp"
namespace catalog_animal_repair_premium_m2_compositions::animal_groups_policy {
namespace {
#include "data.inc"
}
const Library& library(){
    static const Library result=[] {
        Library result;const int* p=data;
        for(auto& family:result){
            family.first=*p++;family.count=*p++;
            for(int c=0;c<family.count;++c)for(int leaf=0;leaf<2;++leaf){
                auto& course=family.choices[c][leaf];course.days.reserve(30-family.first);
                for(int day=family.first;day<30;++day){
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
                prepare(course);
            }
        }
        if(p!=data+std::size(data))std::abort();return result;
    }();
    return result;
}
}
