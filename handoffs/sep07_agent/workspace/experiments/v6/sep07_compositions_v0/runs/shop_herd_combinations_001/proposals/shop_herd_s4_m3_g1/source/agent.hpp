#pragma once
#include "../../../../../include/shop_herd.hpp"
namespace compositions::shop_herd_s4_m3_g1 {
class Agent:public ShopHerdAgent {public:Agent():ShopHerdAgent({AdaptiveAnimalPlan{AnimalEdit{10,10,{176,1},{180,3},{183,3},{155,2},25},7,30,Service{0,536868608u,805306240u,1073741568u,715816960u,0}}},3,1){}
static kag::agent::AgentInfo info(){return {"shop_herd_s4_m3_g1"};}};}
