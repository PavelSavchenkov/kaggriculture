#pragma once
#include "../../../../../include/shop_herd.hpp"
namespace compositions::shop_herd_s2_m3_g0 {
class Agent:public ShopHerdAgent {public:Agent():ShopHerdAgent({AdaptiveAnimalPlan{AnimalEdit{10,10,{169,2},{175,3},{177,3},{156,4},46},7,30,Service{0,536868608u,536870784u,1073741568u,715816960u,0}}},3,0){}
static kag::agent::AgentInfo info(){return {"shop_herd_s2_m3_g0"};}};}
