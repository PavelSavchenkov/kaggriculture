#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_one_fert/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_one_plain/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_three_fert/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/productive_wheat_rotation_001/proposals/wheat_three_plain/source/agent.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char**argv){
    if(argc!=2)return 2;using namespace compositions;int actions=0,guards=0;
    auto check=[&](std::string name,const std::vector<GuardedDay>&days){
        for(const auto&g:days){const auto folder=std::filesystem::path(argv[1])/name/"days"/std::to_string(g.plan.day);
            std::ifstream a(folder/"actions.txt"),s(folder/"guard.txt");int v;
            auto equal=[&](std::istream&in,int expected){in>>v;if(!in||v!=expected)std::abort();};
            for(const auto&x:g.plan.actions){equal(a,x.n_units);equal(a,x.n_orders);
                for(int u=0;u<x.n_units;++u){equal(a,x.units[u].op);equal(a,x.units[u].arg);equal(a,x.units[u].n);}
                for(int i=0;i<x.n_orders;++i){equal(a,x.orders[i].op);equal(a,x.orders[i].item);equal(a,x.orders[i].n);}++actions;}
            equal(s,g.plan.day);equal(s,g.quadrants);for(int cell=0;cell<100;++cell){equal(s,g.check[cell]);for(int value:g.tiles[cell])equal(s,value);}
            for(int value:g.shed)equal(s,value);for(int value:g.seeds)equal(s,value);++guards;
        }
    };
    check("one_fert",wheat_one_fert::off_days());check("one_fert_berry",wheat_one_fert::on_days());
    check("one_plain",wheat_one_plain::off_days());check("one_plain_berry",wheat_one_plain::on_days());
    check("three_fert",wheat_three_fert::off_days());check("three_fert_berry",wheat_three_fert::on_days());
    check("three_plain",wheat_three_plain::off_days());check("three_plain_berry",wheat_three_plain::on_days());
    std::cout<<"{\"exact_source_actions\":"<<actions<<",\"exact_source_guards\":"<<guards<<"}\n";
}
