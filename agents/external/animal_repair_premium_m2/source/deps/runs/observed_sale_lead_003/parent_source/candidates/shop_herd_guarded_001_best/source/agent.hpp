#pragma once
#include "../../../runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp"
#include "../../../runs/shop_herd_day_library_001/days.hpp"
namespace catalog_animal_repair_premium_m2_sale::shop_herd_guarded_001_best {
class Agent:public GuardedDayAgent<shop_herd_s6_m3_g1::Agent> {public:Agent():GuardedDayAgent<shop_herd_s6_m3_g1::Agent>(shop_herd_days::select({8,9,11,13,14,15,16,18,19,20,23,24,25,26,27})){}
static kag::agent::AgentInfo info(){return {"shop_herd_guarded_001_best"};}};
}
