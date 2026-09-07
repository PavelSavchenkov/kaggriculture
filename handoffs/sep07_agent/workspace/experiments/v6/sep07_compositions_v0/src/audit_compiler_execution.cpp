#include "../include/evaluation.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include "../candidates/composition_greedy_v1/source/agent.hpp"
#include "../candidates/opening_router_v3/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

int main(int argc,char** argv) {
    if(argc<2) {std::cerr<<"usage: audit_compiler_execution DIRECTORY [arena options]\n";return 2;}
    const fs::path directory=argv[1];if(fs::exists(directory) && !fs::is_empty(directory))return 2;
    fs::create_directories(directory);auto config=options(argc-1,argv+1);config.validate=true;
    int matched=0;
    for(int program:{0,4,55}) {
        for(int seat=0;seat<2;++seat) {
            greedy::AgentCore old(program,true,true,true,1,true);
            greedy_v1::AgentCore unchanged(program,true,true,true,1,true,0);
            public_router::Agent opponent;
            const auto a=run_game(old,opponent,1000,seat,config);
            const auto b=run_game(unchanged,opponent,1000,seat,config);
            for(int p=0;p<2;++p)if(a.hash[p]!=b.hash[p] || a.cash[p]!=b.cash[p])std::abort();
            ++matched;
        }
        for(int mode=0;mode<4;++mode) {
            config.a="compiler_p"+std::to_string(program)+"_m"+std::to_string(mode);
            auto own=[=]{return greedy_v1::AgentCore(program,true,true,true,1,true,mode);};
            config.b="public_router";config.output=(directory/(config.a+"_vs_public.json")).string();
            run_batch(config,own,[]{return public_router::Agent{};});
            config.b="opening_router_v3";config.output=(directory/(config.a+"_vs_v3.json")).string();
            run_batch(config,own,[]{return opening_router_v3::Agent{};});
        }
    }
    std::ofstream(directory/"parity.json")<<"{\"old_mode0_identical_games\":"<<matched<<"}\n";
}
