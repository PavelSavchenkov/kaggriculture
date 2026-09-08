#pragma once
#include "../../../../../include/crop_rotation_shop_gate.hpp"
#include "../../../../crop_value_001/proposals/crop_value_m2_t4/source/agent.hpp"
#include "../../../../crop_rotation_009/agent/source/agent.hpp"
namespace compositions::crop_rotation_t2_current {class Agent:public CropRotationShopGate<compositions::crop_value_m2_t4::Agent,crop_rotation_009::Agent>{public:Agent():CropRotationShopGate(2){}static kag::agent::AgentInfo info(){return {"crop_rotation_t2_current"};}};}
