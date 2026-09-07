#pragma once
#include "../../../../../include/day_override.hpp"
namespace compositions::reduce_hires_001_25 {
inline std::array<kag::Action,24> schedule() {
constexpr int data[]={
1,6,4,0,1,1,0,1,1,0,1,1,0,1,0,0,0,3,0,5,3,1,3,
4,0,3,0,1,0,0,1,4,0,1,1,0,1,
4,0,5,0,1,5,0,1,5,0,1,5,0,2,
4,1,15,0,1,4,0,1,1,0,1,1,0,1,6,8,1,
4,1,17,0,1,17,0,1,0,0,1,15,0,1,3,3,1,
4,1,16,0,1,15,0,1,0,0,1,17,0,1,6,0,2,
4,0,6,0,1,16,0,1,16,0,1,4,0,1,
4,0,1,0,1,4,0,1,2,0,1,17,0,1,
4,0,1,0,1,4,0,1,7,8,1,15,0,1,
4,0,17,0,1,4,0,1,1,0,1,16,0,1,
4,0,4,0,1,1,0,1,1,0,1,3,0,1,
4,0,9,0,1,9,0,1,16,0,1,2,0,1,
4,1,3,0,1,1,0,1,4,0,1,2,0,1,4,0,1,
4,0,1,0,1,1,0,1,4,0,1,6,0,1,
4,1,9,0,1,9,0,1,2,0,1,0,0,1,4,0,1,
4,1,4,0,1,10,0,1,2,0,1,0,0,1,6,8,1,
4,1,4,0,1,8,0,1,17,0,1,0,0,1,4,0,1,
4,0,1,0,1,9,0,1,15,0,1,0,0,1,
4,1,9,0,1,1,0,1,16,0,1,0,0,1,4,0,1,
4,0,4,0,1,0,0,1,3,0,1,0,0,1,
4,1,9,0,1,9,0,1,3,0,1,0,0,1,4,0,1,
4,0,10,0,1,10,0,1,3,0,1,0,0,1,
4,3,8,0,1,8,0,1,6,0,1,0,0,1,6,8,4,4,0,1,3,3,3,
4,0,9,0,1,9,0,1,0,0,1,0,0,1,
};
std::array<kag::Action,24> result;const int* p=data;
for(auto& action:result){action.n_units=*p++;action.n_orders=*p++;
for(int u=0;u<action.n_units;++u){action.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}
for(int i=0;i<action.n_orders;++i){action.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}action.finalize();}return result;}
class Agent:public DayOverrideAgent {public:Agent():DayOverrideAgent(55,4,schedule()){}
static kag::agent::AgentInfo info(){return {"reduce_hires_001_25"};}};
}
