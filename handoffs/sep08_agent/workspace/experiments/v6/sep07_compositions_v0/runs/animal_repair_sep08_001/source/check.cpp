#include "policy.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/source/policy.hpp"
#include "experiments/v6/sep07_compositions_v0/include/evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include <iostream>
using namespace compositions;

template<class Rival>
bool fixture(int family,int choice,int leaf,uint64_t seed,double cash,double rival_cash){
    animal_repair::Policy own(1,family,choice,leaf);Rival rival;Options options;options.validate=true;
    const auto result=run_game(own,rival,seed,0,options);const auto& d=own.diagnostics();
    const uint32_t expected=((uint32_t{1}<<30)-1)^((uint32_t{1}<<animal_groups_policy::library()[family].first)-1);
    const bool pass=result.turns==719 && result.cash[0]==cash && result.cash[1]==rival_cash
        && d.family==family && d.choice==choice && d.matched_days==expected && !d.missed_days && !d.repaired_days;
    std::cout<<"fixture,"<<family<<','<<choice<<','<<leaf<<','<<seed<<','<<result.cash[0]<<','<<result.cash[1]<<','<<pass<<'\n';
    return pass;
}
int main(int argc,char** argv){
    if(argc!=2)return 2;std::ifstream input(argv[1]);if(!input)return 2;
    int family,choice,leaf,count=0;uint64_t seed;double cash,rival_cash;std::string opponent;bool passed=true;
    while(input>>family>>choice>>leaf>>seed>>opponent>>cash>>rival_cash){
        if(opponent=="public")passed=fixture<public_router::Agent>(family,choice,leaf,seed,cash,rival_cash)&&passed;
        else if(opponent=="pass")passed=fixture<Pass>(family,choice,leaf,seed,cash,rival_cash)&&passed;
        else return 2;
        ++count;
    }
    if(!input.eof() || count!=12 || !passed)return 3;
    for(int mode=1;mode<=2;++mode){
        animal_groups_policy::Policy original(mode);animal_repair::Policy repaired(mode);king_rc4::Agent a,b;
        Options options;options.validate=true;
        const auto before=run_game(original,a,1014,1,options),after=run_game(repaired,b,1014,1,options);
        const auto& d=repaired.diagnostics();
        const uint32_t all=((uint32_t{1}<<30)-1)^((uint32_t{1}<<15)-1);
        bool valid=after.turns==719 && d.family==0 && d.choice==2 && d.matched_days==all
            && d.repaired_days==(uint32_t{1}<<23) && !d.missed_days;
        for(int p=0;p<kag::N_ITEMS;++p)valid&=after.produced[1][p]-before.produced[1][p]==(p==kag::WHEAT?6:0);
        valid&=after.worker_days[1]==before.worker_days[1];
        if(mode==1)valid&=before.cash[1]==133622 && before.cash[0]==83734 && after.cash[1]==133774 && after.cash[0]==83714;
        std::cout<<"repair,"<<mode<<','<<before.cash[1]<<','<<after.cash[1]<<','<<before.cash[0]<<','<<after.cash[0]
            <<','<<before.faults[1]<<','<<after.faults[1]<<','<<after.hash[1]<<','<<valid<<'\n';
        if(!valid)return 4;
    }
}
