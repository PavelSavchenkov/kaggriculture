#include "repair.hpp"
namespace catalog_animal_repair_premium_m2_compositions::animal_repair {
namespace {
#include "repair_data.inc"
}
const GuardedDay& repair_day(){
    static const GuardedDay result=[] {
        GuardedDay g{};const int* p=data;g.plan.day=*p++;g.quadrants=*p++;
        for(int c=0;c<100;++c){g.check[c]=*p++;for(auto& x:g.tiles[c])x=*p++;}
        for(auto& x:g.shed)x=*p++;for(auto& x:g.seeds)x=*p++;
        for(auto& a:g.plan.actions){
            a.n_units=*p++;a.n_orders=*p++;
            for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            a.finalize();
        }
        if(p!=data+std::size(data) || g.plan.day!=23)std::abort();return g;
    }();return result;
}
}
