#pragma once
#include "../../../../../include/crop_rotation_shop_gate.hpp"
#include "../../../../../candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "../../../../crop_rotation_009/agent/source/agent.hpp"
namespace compositions::crop_rotation_t2 {class Agent:public CropRotationShopGate<compositions::investment_context_guarded_001_best::Agent,crop_rotation_009::Agent>{public:Agent():CropRotationShopGate(2){}static kag::agent::AgentInfo info(){return {"crop_rotation_t2"};}};}
