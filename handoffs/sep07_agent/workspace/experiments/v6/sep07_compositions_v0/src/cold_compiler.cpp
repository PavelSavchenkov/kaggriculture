#include "../include/evaluation.hpp"
#include "../include/biology.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include <filesystem>

using namespace compositions;

std::vector<Life> cold_farm(int cows,int sheep,int geese,int wheat,int melon,int strawberry) {
    std::vector<Life> lives;
    auto add=[&](int item,int count,int day,int end) {
        for(int i=0;i<count;++i)lives.push_back({item,day*24,std::min(719,end*24),0,0});
    };
    add(kag::COW,cows,0,30);add(kag::SHEEP,sheep,0,30);add(kag::GOOSE,geese,3,30);
    add(kag::MELON,melon,0,13);
    add(kag::STRAWBERRY,strawberry,5,22);
    for(int day=0;day<=24;day+=5)add(kag::WHEAT,wheat,day,day+5);
    std::stable_sort(lives.begin(),lives.end(),[](const Life& a,const Life& b){return a.start<b.start;});
    return lives;
}

int main(int argc,char** argv) {
    if(argc<2)return 2;
    std::filesystem::path directory=argv[1];std::filesystem::create_directories(directory);
    auto options=compositions::options(argc-1,argv+1);options.b="public_router";
    struct Case {const char* name;int cows,sheep,geese,wheat,melon,strawberry;};
    const Case cases[]={{"cold_wheat",0,0,0,20,0,0},{"cold_goose",0,0,6,12,0,0},
        {"cold_dairy",5,0,0,12,0,0},{"cold_wool",0,4,0,12,0,0},
        {"cold_mixed",2,2,0,7,12,8},{"cold_no_wheat",2,2,0,0,12,12}};
    for(const auto& c:cases) {
        auto proposal=cold_farm(c.cows,c.sheep,c.geese,c.wheat,c.melon,c.strawberry);
        options.a=c.name;options.output=(directory/(options.a+".json")).string();
        run_batch(options,[&]{return greedy::AgentCore(proposal,Support{},false,false,false,1,true);},[]{return public_router::Agent{};});
    }
    // API parity: arbitrary typed input must preserve a recorded proposal when
    // source layout/support and service settings are identical.
    auto source=greedy::recorded_program(0);std::vector<Life> proposal(source.begin(),source.end());
    options.a="custom_program0";options.output=(directory/(options.a+".json")).string();
    run_batch(options,[&]{return greedy::AgentCore(proposal,greedy::recorded_support(0),true,true,false,1,true);},[]{return public_router::Agent{};});
}
