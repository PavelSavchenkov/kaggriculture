#include "../include/evaluation.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../league/top_replay_library/source/agent.hpp"
#include "../candidates/opening_router_v0/source/agent.hpp"
#include "../candidates/opening_router_v1/source/agent.hpp"
#include "../candidates/opening_router_v2/source/agent.hpp"
#include "../candidates/opening_router_v3/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include "../candidates/justin_recall_v0/source/agent.hpp"
#include <filesystem>

// Exact compiler calibration over many dated programs. No source program is
// selected from privileged game state; every match starts with a fixed choice.
int main(int argc,char** argv) {
    if(argc<5) {
        std::fprintf(stderr,"usage: evaluate_library FIRST_PROGRAM LAST_PROGRAM SERVICE_MODE OUTPUT_DIRECTORY [arena options]\n");
        return 2;
    }
    int first=std::stoi(argv[1]),last=std::stoi(argv[2]),service=std::stoi(argv[3]);
    if(first<0 || first>last || last>=(service==2?compositions::top_replay_library::program_count():72) || service<0 || service>2)return 2;
    std::filesystem::path directory=argv[4];std::filesystem::create_directories(directory);
    auto options=compositions::options(argc-4,argv+4);
    if(options.b=="pass")options.b="public_router";
    if(options.b!="public_router" && options.b!="opening_router_v0" && options.b!="opening_router_v1" && options.b!="opening_router_v2" && options.b!="opening_router_v3" && options.b!="opening_router_v4" && options.b!="justin_recall_v0")return 2;
    if(service!=2 && options.b!="public_router")return 2;
    for(int program=first;program<=last;++program) {
        options.a="program_"+std::to_string(program)+(service==2?"_teacher":service?"_source":"_productive");
        options.output=(directory/(options.a+".json")).string();
        if(service==2 && options.b=="justin_recall_v0")
            compositions::run_batch(options,[&]{return compositions::top_replay_library::Agent(program);},
                []{return compositions::justin_recall_v0::Agent{};});
        else if(service==2 && options.b=="opening_router_v4")
            compositions::run_batch(options,[&]{return compositions::top_replay_library::Agent(program);},
                []{return compositions::opening_router_v4::Agent{};});
        else if(service==2 && options.b=="opening_router_v3")
            compositions::run_batch(options,[&]{return compositions::top_replay_library::Agent(program);},
                []{return compositions::opening_router_v3::Agent{};});
        else if(service==2 && options.b=="opening_router_v2")
            compositions::run_batch(options,[&]{return compositions::top_replay_library::Agent(program);},
                []{return compositions::opening_router_v2::Agent{};});
        else if(service==2 && options.b=="opening_router_v1")
            compositions::run_batch(options,[&]{return compositions::top_replay_library::Agent(program);},
                []{return compositions::opening_router_v1::Agent{};});
        else if(service==2 && options.b=="opening_router_v0")
            compositions::run_batch(options,[&]{return compositions::top_replay_library::Agent(program);},
                []{return compositions::opening_router_v0::Agent{};});
        else if(service==2)
            compositions::run_batch(options,[&]{return compositions::top_replay_library::Agent(program);},
                []{return compositions::public_router::Agent{};});
        else
            compositions::run_batch(options,[&]{return compositions::greedy::AgentCore(program,true,true,service,1,true);},
                []{return compositions::public_router::Agent{};});
    }
}
