#include "experiments/v6/sep07_compositions_v0/runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp"
#include <iostream>
namespace source_tapes {
#include "experiments/v6/sep07_compositions_v0/league/top_replay_library/source/tapes.inc"
}
template<class T> void array(const T& values) {
    std::cout << '[';bool first=true;for(auto value:values){if(!first)std::cout<<',';first=false;std::cout<<value;}std::cout<<']';
}
void action(const kag::Action& a) {
    std::cout<<"[[";
    for(int u=0;u<a.n_units;++u){if(u)std::cout<<',';std::cout<<'['<<+a.units[u].op<<','<<+a.units[u].arg<<','<<a.units[u].n<<']';}
    std::cout<<"],[";
    for(int i=0;i<a.n_orders;++i){if(i)std::cout<<',';std::cout<<'['<<+a.orders[i].op<<','<<+a.orders[i].item<<','<<a.orders[i].n<<']';}
    std::cout<<"]]";
}
int main(){
    std::cout<<"{\"tape\":[";
    for(int step=0;step<719;++step){
        if(step)std::cout<<',';int p=source_tapes::offsets[150][step];kag::Action a;a.clear();
        a.n_units=source_tapes::values[p++];a.n_orders=source_tapes::values[p++];
        for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(source_tapes::values[p]),uint8_t(source_tapes::values[p+1]),source_tapes::values[p+2]};p+=3;}
        for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(source_tapes::values[p]),uint8_t(source_tapes::values[p+1]),source_tapes::values[p+2]};p+=3;}
        action(a);
    }
    std::cout<<"],\"days\":[";bool first=true;
    for(const auto& entry:compositions::day_library::entries()){
        if(!first)std::cout<<',';first=false;const auto& d=entry.day;
        std::cout<<"{\"day\":"<<d.plan.day<<",\"quadrants\":"<<d.quadrants<<",\"shed\":";array(d.shed);
        std::cout<<",\"seeds\":";array(d.seeds);std::cout<<",\"tiles\":[";bool cellfirst=true;
        for(int cell=0;cell<100;++cell)if(d.check[cell]){if(!cellfirst)std::cout<<',';cellfirst=false;std::cout<<'['<<cell<<',';array(d.tiles[cell]);std::cout<<']';}
        std::cout<<"],\"actions\":[";for(int h=0;h<24;++h){if(h)std::cout<<',';action(d.plan.actions[h]);}std::cout<<"]}";
    }
    std::cout<<"],\"shop_mask\":";array(kag::SHOP_MASK);std::cout<<",\"shop_mult\":";array(kag::SHOP_MULT);std::cout<<"}\n";
}
