#include "../../include/evaluation.hpp"
#include "../../league/public_router/source/agent.hpp"
#include "proposals/compiler_placement_mixed_m0/source/agent.hpp"
#include "proposals/compiler_placement_mixed_m1/source/agent.hpp"
#include <filesystem>

using namespace compositions;

template<class Base> struct Trace {
    Base base;
    std::ostream* output;
    explicit Trace(std::ostream& stream):output(&stream) {}
    void reset(const kag::agent::AgentInit& init){base.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        base.act(o,b,a);
        if(o.day>9)return;
        *output<<"{\"step\":"<<o.step<<",\"cash\":"<<o.self().money<<",\"quadrants\":"<<o.self().n_quadrants
            <<",\"hires\":"<<o.self().hires_today<<",\"shed\":[";
        for(int i=0;i<kag::N_ITEMS;++i){if(i)*output<<',';*output<<o.own.shed[i];}
        *output<<"],\"orders\":[";
        for(int i=0;i<a.n_orders;++i){const auto& v=a.orders[i];if(i)*output<<',';*output<<'['<<+v.op<<','<<+v.item<<','<<v.n<<']';}
        *output<<"]}\n";
    }
};

template<class Agent> void run(const std::filesystem::path& directory,int mode) {
    std::ofstream trace(directory/("mode"+std::to_string(mode)+".jsonl"));
    Trace<Agent> own(trace);public_router::Agent rival;
    Options options;options.a="compiler_placement_mixed_m"+std::to_string(mode);options.b="public_router";
    options.validate=true;options.profile=true;options.output=(directory/("mode"+std::to_string(mode)+"_game.json")).string();
    std::vector<Outcome> results;results.push_back(run_game(own,rival,1000,0,options));
    write_results(options,results,0);
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;
    const std::filesystem::path directory=argv[1];if(std::filesystem::exists(directory))return 2;
    std::filesystem::create_directories(directory);
    run<compiler_placement_mixed_m0::Agent>(directory,0);
    run<compiler_placement_mixed_m1::Agent>(directory,1);
}
