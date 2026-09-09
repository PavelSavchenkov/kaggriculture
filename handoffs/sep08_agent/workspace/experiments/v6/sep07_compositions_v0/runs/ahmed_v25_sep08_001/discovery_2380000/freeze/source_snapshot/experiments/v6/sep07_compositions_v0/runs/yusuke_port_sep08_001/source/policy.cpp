#include "policy.hpp"
namespace compositions::yusuke_port {
namespace {
#include "data.inc"
struct Tables {
    std::array<std::array<kag::Action,719>,4> actions{};
    Tables(){
        for(int r=0;r<4;++r)for(int t=0;t<719;++t){
            auto& a=actions[r][t];const int* p=values+offsets[r][t];
            a.n_units=*p++;a.n_orders=*p++;
            for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            a.finalize();
        }
    }
};
}
const kag::Action& planned(int route,int step){static const Tables tables;return tables.actions[route][step];}
}
