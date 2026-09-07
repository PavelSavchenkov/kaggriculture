#pragma once
#include <array>
#include <vector>
#include "agents/common/api/agent_api.hpp"

namespace compositions::greedy {
struct Intent { int item,start,end,x,y; };

class AgentCore {
public:
    AgentCore(int program=0,bool source_layout=true,bool source_support=true)
        : program_id_(program),source_layout_(source_layout),source_support_(source_support) {}
    void reset(const kag::agent::AgentInit& init);
    void act(const kag::agent::AgentObservation& observation,const kag::agent::DecisionBudget&,kag::Action& action);
private:
    int program_id_=0;
    bool source_layout_=true,source_support_=true;
    kag::agent::AgentConfig config_{};
    std::vector<Intent> intents_;
    std::array<int,30> hands_{},quadrants_{};
};

template<int Program=0,bool SourceLayout=true,bool SourceSupport=true>
class Agent:public AgentCore {
public:
    Agent():AgentCore(Program,SourceLayout,SourceSupport) {}
    static kag::agent::AgentInfo info() {return {"composition_greedy"};}
};
}
