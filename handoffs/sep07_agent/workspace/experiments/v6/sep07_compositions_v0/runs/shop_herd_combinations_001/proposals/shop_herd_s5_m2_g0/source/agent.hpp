#pragma once
#include "../../../../../include/shop_herd.hpp"
namespace compositions::shop_herd_s5_m2_g0 {
class Agent:public ShopHerdAgent {public:Agent():ShopHerdAgent({AdaptiveAnimalPlan{AnimalEdit{10,10,{88,1},{92,3},{95,3},{29,3},42},3,30,Service{0,536870640u,268435440u,536870896u,715827200u,0}},AdaptiveAnimalPlan{AnimalEdit{10,10,{176,1},{180,3},{183,3},{155,2},25},7,30,Service{0,536868608u,805306240u,1073741568u,715816960u,0}}},2,0){}
static kag::agent::AgentInfo info(){return {"shop_herd_s5_m2_g0"};}};}
