#include "agent.hpp"
namespace kag::agents::nanare_four_quadrant_course {
namespace {
#include "replay.inc"
}
void Agent::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& a){
 a.clear();if(o.step<0 || o.step>=719)std::abort();int pos=offsets[o.step];const int units=values[pos++];a.n_orders=values[pos++];
 for(int u=0;u<units;++u){a.units[u]={uint8_t(values[pos]),uint8_t(values[pos+1]),values[pos+2]};pos+=3;}
 for(int s=0;s<a.n_orders;++s){a.orders[s]={uint8_t(values[pos]),uint8_t(values[pos+1]),values[pos+2]};pos+=3;}
 a.n_units=o.self().n_units;for(int u=units;u<a.n_units;++u)a.units[u]={};a.finalize();
}
}
