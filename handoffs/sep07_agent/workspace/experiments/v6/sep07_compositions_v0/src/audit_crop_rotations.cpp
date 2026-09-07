#include "../include/count_spans.hpp"
#include "../include/evaluation.hpp"
#include "../candidates/composition_greedy_v1/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

int main(int argc,char** argv) {
    if(argc<2)return 2;
    fs::path directory=argv[1];if(fs::exists(directory) && !fs::is_empty(directory))return 2;
    fs::create_directories(directory);auto config=options(argc-1,argv+1);
    config.validate=config.profile=true;config.b="pass";
    std::ofstream contracts(directory/"contracts.csv");
    contracts<<"case,mode,lives,requested_days,immature_days,biology_melon,water_visits,fertilizer_visits,expand_us\n";
    for(int farm=0;farm<4;++farm)for(int mode=0;mode<4;++mode) {
        const bool early=mode&1,prune=mode&2;
        std::vector<CountSpan> spans={{{kag::MELON,farm==0?2:farm==3?4:10,0,22},early?-2:-1,farm>=2,prune}};
        if(farm==3) {
            spans.push_back({{kag::WHEAT,8,0,30},early?-2:-1,true,prune});
            spans.push_back({{kag::STRAWBERRY,8,0,30},-1,true,prune});
            spans.push_back({{kag::COW,2,0,30}});
            spans.push_back({{kag::SHEEP,2,3,30}});
        }
        const auto start=std::chrono::steady_clock::now();
        auto expanded=expand_count_spans(spans);
        const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        int melon=0,water=0,fertilizer=0;
        for(const auto& life:expanded.lives) {
            const int first=life.start/24,last=std::min(30,(life.end+23)/24);
            const auto b=biology({uint8_t(life.item),1,first,last},{life.water,life.feed,life.care,life.collect,life.harvest,life.fertilize});
            for(const auto& day:b.days)melon+=day.output[kag::MELON];
            if(kag::is_crop(life.item))for(int day=first;day<last;++day){water+=on(life.water,day);fertilizer+=on(life.fertilize,day);}
        }
        if(farm==0 && melon!=(early?24:12))std::abort();
        contracts<<farm<<','<<mode<<','<<expanded.lives.size()<<','<<expanded.requested_count_days<<','<<expanded.immature_count_days<<','<<melon<<','<<water<<','<<fertilizer<<','<<us<<'\n';contracts.flush();
        config.a="farm"+std::to_string(farm)+"_mode"+std::to_string(mode);
        config.output=(directory/(config.a+".json")).string();
        run_batch(config,[&]{return greedy_v1::AgentCore(expanded.lives,Support{},false,false,true,1,true,16);},[]{return Pass{};});
    }
}
