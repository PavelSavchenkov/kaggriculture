#pragma once
#include "../../../../../include/shop_herd.hpp"
namespace compositions::shop_herd_s1_m3_g1 {
class Agent:public ShopHerdAgent {public:Agent():ShopHerdAgent({AdaptiveAnimalPlan{AnimalEdit{10,10,{88,1},{92,3},{95,3},{29,3},42},3,30,Service{0,536870640u,268435440u,536870896u,715827200u,0}}},3,1){}
static kag::agent::AgentInfo info(){return {"shop_herd_s1_m3_g1"};}};}
