#include "policy.hpp"
#include "experiments/v6/sep07_compositions_v0/include/evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/league/public_router/source/agent.hpp"
#include <iostream>
using namespace compositions;
using namespace animal_groups_policy;

template<class Rival>
bool check(int family,int choice,int leaf,uint64_t seed,double cash,double rival_cash){
    Policy own(1,family,choice,leaf);Rival rival;Options options;options.validate=true;
    const auto result=run_game(own,rival,seed,0,options);
    const auto d=own.diagnostics();
    const uint32_t expected=((uint32_t{1}<<30)-1)^((uint32_t{1}<<library()[family].first)-1);
    const bool pass=result.turns==719 && result.cash[0]==cash && result.cash[1]==rival_cash
        && d.family==family && d.choice==choice && d.matched_days==expected && !d.missed_days;
    std::cout<<family<<','<<choice<<','<<leaf<<','<<seed<<','<<result.cash[0]<<','<<result.cash[1]
        <<','<<d.family<<','<<d.choice<<','<<d.matched_days<<','<<d.missed_days<<','<<pass<<'\n';
    return pass;
}
int main(int argc,char** argv){
    if(argc!=2)return 2;std::ifstream input(argv[1]);if(!input)return 2;
    std::cout<<"family,choice,leaf,seed,cash,rival_cash,selected_family,selected_choice,matched_days,missed_days,pass\n";
    int family,choice,leaf,count=0;uint64_t seed;double cash,rival_cash;std::string opponent;bool passed=true;
    while(input>>family>>choice>>leaf>>seed>>opponent>>cash>>rival_cash){
        if(opponent=="public")passed=check<public_router::Agent>(family,choice,leaf,seed,cash,rival_cash)&&passed;
        else if(opponent=="pass")passed=check<Pass>(family,choice,leaf,seed,cash,rival_cash)&&passed;
        else return 2;
        ++count;
    }
    return input.eof() && count==12 && passed?0:3;
}
