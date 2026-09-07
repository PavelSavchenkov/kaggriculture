#include "../include/evaluation.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include "../candidates/composition_greedy_v1/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../league/top_replay_library/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

template<class Base> struct ObservedAgent {
    Base base;
    int moves=0,reversals=0,work=0;
    std::array<int,kag::MAX_UNITS> last{};
    explicit ObservedAgent(Base value):base(std::move(value)) {}
    void reset(const kag::agent::AgentInit& init) {
        base.reset(init);moves=reversals=work=0;last.fill(0);
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        base.act(o,b,a);if(o.hour==0)last.fill(0);
        for(int u=0;u<a.n_units;++u) {
            const int op=a.units[u].op,previous=last[u];
            if(op>=kag::OP_NORTH && op<=kag::OP_WEST) {
                ++moves;
                reversals+=(op==kag::OP_NORTH && previous==kag::OP_SOUTH)
                    || (op==kag::OP_SOUTH && previous==kag::OP_NORTH)
                    || (op==kag::OP_EAST && previous==kag::OP_WEST)
                    || (op==kag::OP_WEST && previous==kag::OP_EAST);
            } else if(op!=kag::OP_PASS)++work;
            last[u]=op;
        }
    }
};

int main(int argc,char** argv) {
    if(argc<2){std::cerr<<"usage: audit_compiler_routing DIRECTORY [arena options]\n";return 2;}
    const fs::path directory=argv[1];if(fs::exists(directory) && !fs::is_empty(directory))return 2;
    fs::create_directories(directory);auto config=options(argc-1,argv+1);config.validate=true;
    config.b="public_router";
    std::ofstream diagnostics(directory/"movement.csv");
    diagnostics<<"program,mode,seed,seat,moves,reversals,work\n";
    int parity=0;
    for(int program:{0,4,55}) {
        for(int seat=0;seat<2;++seat) {
            greedy::AgentCore old(program,true,true,true,1,true);
            greedy_v1::AgentCore unchanged(program,true,true,true,1,true,0);
            public_router::Agent opponent;
            const auto a=run_game(old,opponent,1000,seat,config),b=run_game(unchanged,opponent,1000,seat,config);
            for(int p=0;p<2;++p)if(a.hash[p]!=b.hash[p] || a.cash[p]!=b.cash[p])std::abort();
            ++parity;
        }
        for(int mode:{-1,0,1,4,8,5,9}) {
            config.a="routing_p"+std::to_string(program)+"_m"+std::to_string(mode);
            config.output=(directory/(config.a+".json")).string();
            std::vector<Outcome> results;
            const auto start=std::chrono::steady_clock::now();
            auto evaluate=[&](auto own) {
                public_router::Agent opponent;
                const int seats=config.seat_mode==2?2:1;
                for(uint64_t seed:config.seeds)for(int s=0;s<seats;++s) {
                    const int seat=seats==2?s:config.seat_mode;
                    results.push_back(run_game(own,opponent,seed,seat,config));
                    diagnostics<<program<<','<<mode<<','<<seed<<','<<seat<<','<<own.moves<<','<<own.reversals<<','<<own.work<<'\n';
                }
            };
            if(mode==-1)evaluate(ObservedAgent{top_replay_library::Agent(program)});
            else evaluate(ObservedAgent{greedy_v1::AgentCore(program,true,true,true,1,true,mode)});
            write_results(config,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
            diagnostics.flush();
        }
    }
    std::ofstream(directory/"parity.json")<<"{\"old_mode0_identical_games\":"<<parity<<"}\n";
}
