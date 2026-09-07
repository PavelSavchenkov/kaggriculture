#include "../include/count_spans.hpp"
#include "../include/evaluation.hpp"
#include "../candidates/composition_greedy_v0/source/agent.hpp"
#include "../candidates/composition_greedy_v1/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

int main(int argc,char** argv) {
    if(argc<2){std::cerr<<"usage: audit_count_spans DIRECTORY [arena options]\n";return 2;}
    const fs::path directory=argv[1];if(fs::exists(directory) && !fs::is_empty(directory))return 2;
    fs::create_directories(directory);auto config=options(argc-1,argv+1);config.validate=config.profile=true;
    config.b="pass";
    std::ofstream contracts(directory/"contracts.csv");
    contracts<<"name,lives,requested_count_days,immature_count_days,biological_wheat,biological_milk,biological_wool\n";
    const std::vector<std::vector<CountSpan>> cases={
        {{{kag::WHEAT,2,0,20},-1,true}},
        {{{kag::WHEAT,2,0,20},2,true}},
        {{{kag::WHEAT,8,0,30},-1,true},{{kag::COW,2,0,30}},{{kag::SHEEP,2,3,30}},{{kag::STRAWBERRY,8,0,30},-1,true}}
    };
    const char* names[]={"maintained_wheat","short_rotation_wheat","maintained_mixed"};
    for(int index=0;index<int(cases.size());++index) {
        const auto expanded=expand_count_spans(cases[index]);
        std::array<std::array<int,30>,kag::N_ITEMS> requested{},actual{};
        for(const auto& span:cases[index])for(int day=span.occupancy.start_day;day<span.occupancy.end_day;++day)
            requested[span.occupancy.item][day]+=span.occupancy.count;
        int outputs[kag::N_PRODUCTS]{};
        for(const auto& life:expanded.lives) {
            const int start=life.start/24,end=std::min(30,(life.end+23)/24);
            for(int day=start;day<end;++day)++actual[life.item][day];
            const auto b=biology({uint8_t(life.item),1,start,end},{life.water,life.feed,life.care,life.collect,life.harvest,life.fertilize});
            for(const auto& day:b.days)for(int item=0;item<kag::N_PRODUCTS;++item)outputs[item]+=day.output[item];
        }
        if(actual!=requested)std::abort();
        if(index==0 && (expanded.lives.size()!=8 || expanded.requested_count_days!=40 || outputs[kag::WHEAT]!=48))std::abort();
        if(index==1 && (expanded.lives.size()!=14 || expanded.immature_count_days!=4 || outputs[kag::WHEAT]!=36))std::abort();
        contracts<<names[index]<<','<<expanded.lives.size()<<','<<expanded.requested_count_days<<','<<expanded.immature_count_days
            <<','<<outputs[kag::WHEAT]<<','<<outputs[kag::MILK]<<','<<outputs[kag::WOOL]<<'\n';contracts.flush();
        for(int mode:{0,16}) {
            config.a=names[index];if(mode)config.a+="_m"+std::to_string(mode);
            config.output=(directory/(config.a+".json")).string();
            run_batch(config,[&]{return greedy_v1::AgentCore(expanded.lives,Support{},false,false,true,1,true,mode);},[]{return Pass{};});
        }
    }
    // Refresh and added target memory must preserve all old facts and mode0.
    int matched=0;
    for(int program:{0,4,55})for(int seat=0;seat<2;++seat) {
        greedy::AgentCore old(program,true,true,true,1,true);
        greedy_v1::AgentCore unchanged(program,true,true,true,1,true,0);Pass opponent;
        const auto a=run_game(old,opponent,1000,seat,config),b=run_game(unchanged,opponent,1000,seat,config);
        for(int p=0;p<2;++p)if(a.hash[p]!=b.hash[p] || a.cash[p]!=b.cash[p])std::abort();
        ++matched;
    }
    uint64_t checksum=0;
    const auto start=std::chrono::steady_clock::now();
    for(int i=0;i<10000;++i) {
        const auto expanded=expand_count_spans(cases[i%3]);
        checksum+=expanded.lives.size()+expanded.requested_count_days+expanded.immature_count_days;
    }
    const double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/10000;
    std::ofstream(directory/"validation.json")<<"{\"old_mode0_equal_games\":"<<matched
        <<",\"program_count\":"<<greedy_v1::program_count()<<",\"span_cases\":3,\"expansion_microseconds\":"<<us
        <<",\"checksum\":"<<checksum<<",\"scope\":\"Requested occupancy expansion exact; runtime service and occupancy must be assessed from profiles.\"}\n";
}
