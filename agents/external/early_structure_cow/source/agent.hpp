#pragma once
#include "deps/parent/agent.hpp"
#include "structure_repair.hpp"
namespace kag::agents::early_structure_cow {
class Agent:public ::compositions::early_structure_repair::Policy<kag::agents::early_structure_cow_parent::Agent> {
public: static kag::agent::AgentInfo info(){return {"early_structure_cow"};}
};
}
