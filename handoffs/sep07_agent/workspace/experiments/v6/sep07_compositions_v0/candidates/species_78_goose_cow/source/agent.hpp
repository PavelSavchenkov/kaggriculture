#pragma once
#include "../../../include/species_template.hpp"
namespace compositions::species_78_goose_cow {
class Agent:public SpeciesTemplateAgent {
public:
Agent():SpeciesTemplateAgent(78,2,false) {}
static kag::agent::AgentInfo info(){return {"species_78_goose_cow"};}
};
}
