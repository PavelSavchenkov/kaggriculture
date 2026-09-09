#include "lots.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
using namespace compositions::titan_lots;

int main(int argc,char** argv) {
    if(argc!=2)return 2;std::ifstream input(argv[1]);int count=0;input>>count;
    std::vector<Context> cases(count);std::vector<int> capacity(count);
    for(int k=0;k<count;++k) {
        auto& c=cases[k];input>>c.item>>c.quantity>>c.inventory>>c.now>>c.last>>c.rival_quantity>>c.minimum_now>>c.n_dates;
        for(int i=0;i<c.n_dates;++i)input>>c.dates[i];input>>c.n_shops;
        for(int i=0;i<c.n_shops;++i)input>>c.shops[i];input>>c.reference.count;
        for(int i=0;i<c.reference.count;++i)input>>c.reference.sales[i].step>>c.reference.sales[i].quantity;
        input>>capacity[k];if(!input)return 2;
    }
    int64_t checksum=0,plans=0;constexpr int repeats=30;
    const auto start=std::chrono::steady_clock::now();
    for(int r=0;r<repeats;++r)for(int k=0;k<count;++k) {
        const auto value=optimize(cases[k],[&](const Plan& p){return p.at(cases[k].now)>=capacity[k];});
        checksum+=value.worst_gain+value.plan.at(cases[k].now);plans+=value.plans;
    }
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"{\"contexts\":"<<count*repeats<<",\"seconds\":"<<seconds<<",\"microseconds_per_context\":"
        <<1e6*seconds/(count*repeats)<<",\"candidate_plans\":"<<plans<<",\"checksum\":"<<checksum<<"}\n";
}
