#include "case.hpp"
#include "verify.hpp"
#include "repack.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
using namespace kag;
using namespace kag::agents::day_policy_contract;
using contract_benchmark::Case;
int main(int argc,char** argv) {
    if(argc<3)throw std::runtime_error("evaluate cases.bin output.csv [limit] [max_hires] [effort: 0 Full, 1 Fast, 2 Compact, 3 Balanced, 4 Classic, 5 DayPolicy80p] [returns: 0 strict, 1 last hour, 2 windows] [animal_reserve] [variants] [minimize_variants] [repack] [placement_style] [minimize_hires]");
    std::ifstream input(argv[1],std::ios::binary);std::ofstream out(argv[2]);
    const int limit=argc>3?std::stoi(argv[3]):1000000;
    SolveOptions options;
    if(argc>4)options.max_hires=std::stoi(argv[4]);
    if(argc>5) {
        const int effort=std::stoi(argv[5]);
        if(effort<0 || effort>5)throw std::runtime_error("invalid effort");
        const SearchEffort profiles[]={SearchEffort::Full,SearchEffort::Fast,SearchEffort::Compact,SearchEffort::Balanced,SearchEffort::Classic,SearchEffort::DayPolicy80p};
        options.effort=profiles[effort];
    }
    const int return_mode=argc>6?std::stoi(argv[6]):0;
    if(argc>7)options.animal_reserve=std::stoi(argv[7]);
    if(argc>8)options.variants=std::stoi(argv[8]);
    if(argc>9)options.minimize_variants=std::stoi(argv[9]);
    if(argc>11)options.placement=static_cast<PlacementStyle>(std::stoi(argv[11]));
    if(argc>12)options.minimize_hires=std::stoi(argv[12]);
    auto solver=std::make_unique<Solver>();Case c;int eligible=0,ok=0;double micros=0;
    out<<"game,seat,rank,day,original_hires,work,returns,status,hires,attempts,microseconds,hash\n";
    while(input.read(reinterpret_cast<char*>(&c),sizeof(c))) {
        if(std::strcmp(c.reason,"eligible"))continue;
        if(eligible++>=limit)break;
        if(argc>10 && std::stoi(argv[10]))c.input=contract_benchmark::repack(c.input);
        if(return_mode==1)for(int h=0;h<23;++h)std::fill_n(c.input.returns[h],N_PRODUCTS,0);
        if(return_mode==2) {
            auto original=c.input;
            for(int h=0;h<24;++h)for(int it=0;it<N_PRODUCTS;++it)c.input.returns[h][it]=h<8?0:original.returns[h<16?8:h<22?16:h<23?22:23][it];
        }
        if(std::getenv("DAY_POLICY_TRACE"))std::cerr<<"CASE "<<c.game<<" day="<<c.day<<" rank="<<c.rank<<"\n";
        auto result=solver->solve(c.input,options);
        uint64_t hash=14695981039346656037ull;
        if(result.status==SolveStatus::Success) {
            auto checked=verify(c.input,result);if(!checked.valid)throw std::runtime_error(checked.reason);
            for(int h=0;h<24;++h)for(int it=0;it<N_PRODUCTS;++it)if(checked.receipts[h][it]!=result.receipts[h][it])throw std::runtime_error("receipt output mismatch");
            for(const auto& a:result.schedule) {
                auto add=[&](int v){hash^=uint32_t(v);hash*=1099511628211ull;};add(a.n_units);add(a.n_orders);
                for(int u=0;u<a.n_units;++u){add(a.units[u].op);add(a.units[u].arg);add(a.units[u].n);}
                for(int k=0;k<a.n_orders;++k){add(a.orders[k].op);add(a.orders[k].item);add(a.orders[k].n);}
            }
            ++ok;
        }
        micros+=result.microseconds;
        out<<c.game<<','<<c.seat<<','<<c.rank<<','<<c.day<<','<<c.original_hires<<','<<c.work<<','<<c.early<<','<<int(result.status)<<','<<result.hires<<','<<result.attempts<<','<<result.microseconds<<','<<hash<<'\n';
        if(eligible%100==0) {out.flush();std::cout<<eligible<<" complete="<<ok<<" mean_us="<<micros/eligible<<'\n'<<std::flush;}
    }
    std::cout<<"Finished "<<std::min(eligible,limit)<<" complete="<<ok<<" total_us="<<micros<<'\n';
}
