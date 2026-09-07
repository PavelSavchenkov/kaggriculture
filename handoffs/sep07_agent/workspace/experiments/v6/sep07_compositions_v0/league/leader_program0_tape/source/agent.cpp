#include "agent.hpp"
#include <algorithm>

namespace compositions::leader_program0_tape {
namespace {
#include "tape.inc"
}

void Agent::act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget&,kag::Action& action) {
    if(o.step<0 || o.step>=719)std::abort();
    action.clear();
    int cursor=offsets[o.step];
    const int units=values[cursor++];action.n_orders=values[cursor++];
    if(units>kag::MAX_UNITS || action.n_orders>10)std::abort();
    for(int i=0;i<units;++i) {
        action.units[i]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;
    }
    for(int i=0;i<action.n_orders;++i) {
        action.orders[i]={uint8_t(values[cursor]),uint8_t(values[cursor+1]),values[cursor+2]};cursor+=3;
    }
    action.n_units=o.self().n_units;
    for(int i=units;i<action.n_units;++i)action.units[i]={};
    action.finalize();
}
}
