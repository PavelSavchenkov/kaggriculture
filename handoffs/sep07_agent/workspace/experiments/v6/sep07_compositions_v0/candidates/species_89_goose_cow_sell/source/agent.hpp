#pragma once
#include "../../../include/species_template.hpp"
namespace compositions::species_89_goose_cow_sell {
class Agent:public SpeciesTemplateAgent {
public:
Agent():SpeciesTemplateAgent(89,2,true) {}
static kag::agent::AgentInfo info(){return {"species_89_goose_cow_sell"};}
};
}
