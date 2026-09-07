#include "../include/evaluation.hpp"
#include "../include/market_course.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../candidates/opening_router_v1/source/agent.hpp"
#include <filesystem>

using namespace compositions;

int main(int argc,char** argv) {
    if(argc<2) {std::fprintf(stderr,"usage: audit_market_courses DIRECTORY [arena options]\n");return 2;}
    const std::filesystem::path directory=argv[1];
    if(std::filesystem::exists(directory) && !std::filesystem::is_empty(directory))return 2;
    std::filesystem::create_directories(directory);
    auto config=options(argc-1,argv+1);
    struct Case {const char* name;int workers,markets,first,last;};
    constexpr Case cases[]={
        {"parent55",55,55,0,718},{"parent85",85,85,0,718},
        {"w55_m85_hour6",55,85,6,6},{"w85_m55_hour6",85,55,6,6},
        {"w55_m85_before_worker_split",55,85,0,583},
        {"w55_m85_all",55,85,0,718},{"w85_m55_all",85,55,0,718},
        {"w55_m85_day0",55,85,0,23},{"w55_m85_after_day0",55,85,24,718}};
    for(const auto& test:cases) {
        config.a=test.name;
        auto make=[&]{return MarketCourseAgent(test.workers,test.markets,test.first,test.last);};
        for(const std::string opponent:{"public_router","opening_router_v1","junghoon_78","mao_85"}) {
            config.b=opponent;config.output=(directory/(config.a+"_vs_"+opponent+".json")).string();
            int status=0;
            if(opponent=="public_router")status=run_batch(config,make,[]{return public_router::Agent{};});
            else if(opponent=="opening_router_v1")status=run_batch(config,make,[]{return opening_router_v1::Agent{};});
            else status=run_batch(config,make,[&]{return top_replay_library::Agent(opponent=="junghoon_78"?78:85);});
            if(status)return status;
        }
    }
}
