#pragma once
#include "policy.hpp"
namespace contract_benchmark {
struct Case {
    kag::agents::day_policy_contract::DayInput input;
    int game=0,seat=0,rank=0,day=0,original_hires=0,work=0,early=0;
    char reason[64]{};
};
}
