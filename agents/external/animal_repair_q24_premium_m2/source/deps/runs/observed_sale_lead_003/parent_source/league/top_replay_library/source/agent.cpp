#include "agent.hpp"

namespace catalog_animal_repair_q24_premium_m2_sale::top_replay_library {
namespace {
#include "tapes.inc"
}
int program_count() {return sizeof(offsets)/sizeof(offsets[0]);}
Agent::Agent(int program):program_(program) {if(program<0 || program>=program_count())std::abort();}
void Agent::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action) {
    if(o.step<0 || o.step>=719)std::abort();
    action.clear();int cursor=offsets[program_][o.step];
    const int units=values[cursor++];action.n_orders=values[cursor++];
    if(units>kag::MAX_UNITS || action.n_orders>10)std::abort();
    for(int i=0;i<units;++i) {action.units[i]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    for(int i=0;i<action.n_orders;++i) {action.orders[i]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;}
    action.n_units=o.self().n_units;
    for(int i=units;i<action.n_units;++i)action.units[i]={};
    action.finalize();
}
}
