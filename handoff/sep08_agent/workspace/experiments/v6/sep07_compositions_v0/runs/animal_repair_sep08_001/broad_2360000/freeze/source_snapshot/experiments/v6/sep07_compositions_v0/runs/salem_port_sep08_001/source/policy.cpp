#include "policy.hpp"
namespace compositions::salem_port {
namespace {
#include "data.inc"
struct Tables {
    std::array<kag::Action,720> actions{};
    Tables(){
        for(int step=0;step<720;++step){
            auto& a=actions[step];const int* p=values+offsets[step];a.n_units=*p++;a.n_orders=*p++;
            for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            for(int j=0;j<a.n_orders;++j){a.orders[j]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
            a.finalize();
        }
    }
};
}
const kag::Action& planned(int step){static const Tables data;return data.actions[std::clamp(step,0,719)];}
}
