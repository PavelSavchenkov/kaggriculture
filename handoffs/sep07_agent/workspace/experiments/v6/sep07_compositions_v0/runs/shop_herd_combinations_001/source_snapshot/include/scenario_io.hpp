#pragma once
#include "economics.hpp"
#include <fstream>
#include <string>
#include <vector>

namespace compositions {
inline std::vector<EconomicScenario> read_scenarios(const std::string& path) {
    std::ifstream input(path);std::string format;input>>format;
    int count=0;bool two_sided=format=="two_sided_v1";
    if(two_sided)input>>count;else count=std::stoi(format);
    if(!input || count<1 || count>10000)std::abort();
    std::vector<EconomicScenario> scenarios(count);
    for(auto& scenario:scenarios) {
        uint64_t seed;int seat,nstock,nfixed,nrival;double cash;
        input>>seed>>seat>>cash>>nstock>>nfixed>>nrival;
        if(two_sided)input>>scenario.rival_fixed_cost;
        for(auto& shop:scenario.shops) {int value;input>>value;shop=value;}
        for(int i=0;i<nstock;++i) {int a,b,c,d;input>>a>>b>>c>>d;}
        for(int i=0;i<nfixed;++i) {int a,b,c;input>>a>>b>>c;}
        for(int i=0;i<nrival;++i) {
            int step,item,bought,sold;input>>step>>item>>bought>>sold;
            if(step<0 || step>=719 || item<0 || item>=kag::N_PRODUCTS)std::abort();
            scenario.rival_buys[step][item]+=bought;scenario.rival_sells[step][item]+=sold;
        }
    }
    if(!input)std::abort();
    return scenarios;
}
}
