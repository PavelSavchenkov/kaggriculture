#include "case.hpp"
#include "day_jobs.hpp"
#include "placement.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace kag;
using namespace kag::agents::day_policy_contract;
int main(int argc,char** argv) {
    if(argc!=5)throw std::runtime_error("placement_cost cases.bin output.csv style repeats");
    std::ifstream input(argv[1],std::ios::binary);std::ofstream out(argv[2]);
    const auto style=static_cast<PlacementStyle>(std::stoi(argv[3]));
    const int repeats=std::stoi(argv[4]);if(repeats<1)throw std::runtime_error("positive repeats required");
    out<<"game,seat,day,new_products,microseconds\n";
    contract_benchmark::Case c;uint64_t checksum=0;
    while(input.read(reinterpret_cast<char*>(&c),sizeof(c))) {
        if(std::strcmp(c.reason,"eligible"))continue;
        auto sim=detail::initial_state(c.input);auto o=agent::runtime::make_observation(sim,0);
        auto base=compile_day_jobs(c.input,11,0,false);
        const auto begin=std::chrono::steady_clock::now();
        for(int n=0;n<repeats;++n) {
            auto plan=base;
            asm volatile("" : : "g"(&plan) : "memory");
            place_day(o,plan,0,true,style);
            for(int j=0;j<plan.count;++j)checksum+=plan.jobs[j].tile+1;
        }
        const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count()/repeats;
        out<<c.game<<','<<c.seat<<','<<c.day<<','<<c.input.establish_count<<','<<us<<'\n';
    }
    std::cout<<"checksum="<<checksum<<'\n';
}
