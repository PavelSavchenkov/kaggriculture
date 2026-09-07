#include "../include/evaluation.hpp"
#include "../include/species_template.hpp"
#include "../candidates/opening_router_v3/source/agent.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include "../include/biology.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

int main(int argc,char** argv) {
    if(argc<2){std::cerr<<"usage: audit_species_templates DIRECTORY [arena options]\n";return 2;}
    const fs::path directory=argv[1];if(fs::exists(directory) && !fs::is_empty(directory))return 2;
    fs::create_directories(directory);auto config=options(argc-1,argv+1);config.validate=true;
    const auto opponent=config.b;
    for(int program:{0,4,55,78,85,89,116,131}) {
        for(int seat=0;seat<2;++seat) {
            top_replay_library::Agent source(program);SpeciesTemplateAgent identity(program,0);
            public_router::Agent other;
            const auto a=run_game(source,other,1000,seat,config),b=run_game(identity,other,1000,seat,config);
            for(int p=0;p<2;++p)if(a.hash[p]!=b.hash[p] || a.cash[p]!=b.cash[p])std::abort();
        }
        for(int variant=0;variant<6;++variant)for(int clear=0;clear<=int(variant!=0);++clear) {
            config.a="species_p"+std::to_string(program)+"_v"+std::to_string(variant)+"_c"+std::to_string(clear);
            config.output=(directory/(config.a+".json")).string();
            auto own=[=]{return SpeciesTemplateAgent(program,variant,clear);};
            if(opponent=="public_router")run_batch(config,own,[]{return public_router::Agent{};});
            else if(opponent=="opening_router_v3")run_batch(config,own,[]{return opening_router_v3::Agent{};});
            else if(opponent=="junghoon_78")run_batch(config,own,[]{return top_replay_library::Agent(78);});
            else {std::cerr<<"unsupported opponent\n";return 2;}
        }
    }
    std::ofstream(directory/"parity.json")<<"{\"identity_source_equal_games\":16}\n";
}
