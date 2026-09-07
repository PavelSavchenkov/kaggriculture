#pragma once
#include "../../justin_recall_v0/source/agent.hpp"
#include "../../../runs/justin_day_library_001/days.hpp"
namespace compositions::justin_guarded_hires_001_best {
class Agent:public GuardedDayAgent<justin_recall_v0::Agent> {public:Agent():GuardedDayAgent<justin_recall_v0::Agent>(day_library::select({0,1,2,4,5,7,9,10,11,12,13,14,15,16,17,18,20,21,22,23,24,25})){}
static kag::agent::AgentInfo info(){return {"justin_guarded_hires_001_best"};}};
}
