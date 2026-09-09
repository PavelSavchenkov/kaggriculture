#pragma once
#include "../../../source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/include/terminal_layer.hpp"
namespace compositions::sheep_yarn2_terminal{class Agent:public TerminalAgent<sheep_portfolio::Agent,3>{public:Agent():TerminalAgent(sheep_portfolio::Agent(2)){}static kag::agent::AgentInfo info(){return {"sheep_yarn2_terminal"};}};}
