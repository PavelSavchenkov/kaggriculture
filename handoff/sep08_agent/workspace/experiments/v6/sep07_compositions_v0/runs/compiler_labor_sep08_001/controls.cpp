#include "../../include/evaluation.hpp"
#include "../../include/estimate.hpp"
#include "../../candidates/composition_greedy_v0/source/agent.hpp"
#include "../../league/top_replay_library/source/agent.hpp"
#include "../../league/public_router/source/agent.hpp"
#include "cold_farm.hpp"
#include <filesystem>

using namespace compositions;

int main(int argc,char** argv) {
    if(argc!=2)return 2;
    const std::filesystem::path directory=argv[1];
    if(std::filesystem::exists(directory))return 2;
    std::filesystem::create_directories(directory);
    Options o;o.validate=true;o.profile=true;o.games=8;o.threads=4;o.b="public_router";
    for(int seed=1000;seed<1008;++seed)o.seeds.push_back(seed);
    std::ofstream expected(directory/"expected.json");expected<<'[';
    int index=0;
    for(const std::string name:{"mixed","goose","p4","p55"}) {
        const int program=name=="p4"?4:name=="p55"?55:-1;
        std::vector<Life> lives;Support support;
        if(program>=0) {
            const auto source=greedy::recorded_program(program);
            lives.assign(source.begin(),source.end());support=greedy::recorded_support(program);
        } else lives=name=="mixed"?compiler_labor_data::cold_farm(2,2,0,7,12,8)
                                   :compiler_labor_data::cold_farm(0,0,6,12,0,0);
        EstimateOptions options;options.recorded_layout=program>=0;
        options.service=program>=0?ServiceModel::Recorded:ServiceModel::Productive;
        const auto start=std::chrono::steady_clock::now();
        const auto plan=estimate_plan(lives,support,options);
        const double micros=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        if(index++)expected<<',';
        expected<<"{\"case\":\""<<name<<"\",\"estimate_us\":"<<micros<<",\"hire_cost\":"<<plan.hire_cost<<",\"produced\":[";
        for(int i=0;i<kag::N_PRODUCTS;++i){if(i)expected<<',';expected<<plan.produced[i];}
        expected<<"],\"hands\":[";
        for(int d=0;d<30;++d){if(d)expected<<',';expected<<plan.hands[d];}
        expected<<"],\"work_turns\":[";
        for(int d=0;d<30;++d){if(d)expected<<',';expected<<plan.work_turns[d];}
        expected<<"],\"lives\":[";
        for(size_t i=0;i<lives.size();++i){const auto& l=lives[i];if(i)expected<<',';expected<<'['<<l.item<<','<<l.start<<','<<l.end<<','<<l.x<<','<<l.y<<']';}
        expected<<"]}";
        o.a="original_"+name;o.output=(directory/(o.a+".json")).string();
        run_batch(o,[&]{return greedy::AgentCore(lives,support,program>=0,false,program>=0,1,true);},[]{return public_router::Agent{};});
        if(program>=0) {
            o.a="source_hiring_"+name;o.output=(directory/(o.a+".json")).string();
            run_batch(o,[&]{return greedy::AgentCore(program,true,true,true,1,true);},[]{return public_router::Agent{};});
            o.a="source_trace_"+name;o.output=(directory/(o.a+".json")).string();
            run_batch(o,[&]{return top_replay_library::Agent(program);},[]{return public_router::Agent{};});
        }
    }
    expected<<"]\n";
}
