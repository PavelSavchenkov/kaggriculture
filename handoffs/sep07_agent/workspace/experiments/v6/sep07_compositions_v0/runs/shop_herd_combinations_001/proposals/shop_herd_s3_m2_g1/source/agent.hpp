#pragma once
#include "../../../../../include/shop_herd.hpp"
namespace compositions::shop_herd_s3_m2_g1 {
class Agent:public ShopHerdAgent {public:Agent():ShopHerdAgent({AdaptiveAnimalPlan{AnimalEdit{10,10,{88,1},{92,3},{95,3},{29,3},42},3,30,Service{0,536870640u,268435440u,536870896u,715827200u,0}},AdaptiveAnimalPlan{AnimalEdit{10,10,{169,2},{175,3},{177,3},{156,4},46},7,30,Service{0,536868608u,536870784u,1073741568u,715816960u,0}}},2,1){}
static kag::agent::AgentInfo info(){return {"shop_herd_s3_m2_g1"};}};}
