#pragma once
#include "../../../../crop_rotation_berry_gate_001/proposals/crop_rotation_t2_berry/source/agent.hpp"
#include "../../../../productive_wheat_rotation_001/proposals/wheat_one_fert/source/agent.hpp"
namespace catalog_empty_sale_floor_m1_sale::crop_mix_t2_wheat {
class Agent:public CropRotationShopGate<wheat_one_fert::Agent,crop_rotation_t2_berry::Course>{public:Agent():CropRotationShopGate(2){}static kag::agent::AgentInfo info(){return {"crop_mix_t2_wheat"};}};
}
