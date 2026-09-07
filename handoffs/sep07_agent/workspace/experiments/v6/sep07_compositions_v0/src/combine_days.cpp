#include "../include/evaluation.hpp"
#include "../include/planned_opening.hpp"
#include "../league/mao_85/source/agent.hpp"
#include "../league/junghoon_78/source/agent.hpp"
#include <filesystem>
#include <iostream>
#include <sstream>

using namespace compositions;
namespace fs=std::filesystem;

DayPlan load_plan(int day,const fs::path& path) {
    DayPlan result;result.day=day;std::ifstream input(path);if(!input)std::abort();
    for(auto& a:result.actions) {
        int units,orders;if(!(input>>units>>orders) || units<1 || units>kag::MAX_UNITS || orders<0 || orders>10)std::abort();
        a.n_units=units;a.n_orders=orders;
        for(int u=0;u<units;++u) {int op,arg,n;if(!(input>>op>>arg>>n))std::abort();a.units[u]={uint8_t(op),uint8_t(arg),n};}
        for(int i=0;i<orders;++i) {int op,item,n;if(!(input>>op>>item>>n))std::abort();a.orders[i]={uint8_t(op),uint8_t(item),n};}
        a.finalize();
    }
    return result;
}

template<class Rival> double evaluate(const std::vector<DayPlan>& plans,const std::string& name,const std::string& opponent,const fs::path& output) {
    Options options;options.a=name;options.b=opponent;options.validate=true;options.output=output.string();
    std::vector<Outcome> results;results.reserve(64);double margin=0;
    PlannedOpeningAgent own(plans);Rival rival;
    const auto start=std::chrono::steady_clock::now();
    for(int seed=1000;seed<1032;++seed)for(int seat=0;seat<2;++seat) {
        auto result=run_game(own,rival,seed,seat,options);
        margin+=result.cash[seat]-result.cash[1-seat];results.push_back(std::move(result));
    }
    write_results(options,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    return margin/results.size();
}

std::array<double,4> evaluate_league(const std::vector<DayPlan>& plans,const std::string& name,const fs::path& directory) {
    return {evaluate<opening_router_v2::Agent>(plans,name,"opening_router_v2",directory/(name+"_vs_v2.json")),
        evaluate<public_router::Agent>(plans,name,"public_router",directory/(name+"_vs_public.json")),
        evaluate<mao_85::Agent>(plans,name,"mao_85",directory/(name+"_vs_mao85.json")),
        evaluate<junghoon_78::Agent>(plans,name,"junghoon_78",directory/(name+"_vs_junghoon.json"))};
}

void export_agent(const std::string& name,const std::vector<DayPlan>& plans,const fs::path& directory,const fs::path& source) {
    if(fs::exists(directory))std::abort();fs::create_directories(directory/"source");
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    std::ofstream header(directory/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<fs::relative(experiment/"include/planned_opening.hpp",directory/"source").generic_string()
        <<"\"\nnamespace compositions::"<<name<<" {\ninline std::vector<DayPlan> plans() {\nconstexpr int data[]={\n";
    for(const auto& plan:plans) {
        header<<plan.day<<",\n";
        for(const auto& action:plan.actions) {
            header<<action.n_units<<','<<action.n_orders<<',';
            for(int u=0;u<action.n_units;++u)header<<+action.units[u].op<<','<<+action.units[u].arg<<','<<action.units[u].n<<',';
            for(int i=0;i<action.n_orders;++i)header<<+action.orders[i].op<<','<<+action.orders[i].item<<','<<action.orders[i].n<<',';
            header<<'\n';
        }
    }
    header<<"};\nconst int* p=data;std::vector<DayPlan> result("<<plans.size()<<");for(auto& plan:result){plan.day=*p++;\n"
        <<"for(auto& a:plan.actions){a.n_units=*p++;a.n_orders=*p++;for(int u=0;u<a.n_units;++u){a.units[u]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}\n"
        <<"for(int i=0;i<a.n_orders;++i){a.orders[i]={uint8_t(p[0]),uint8_t(p[1]),p[2]};p+=3;}a.finalize();}}return result;}\n"
        <<"class Agent:public PlannedOpeningAgent {public:Agent():PlannedOpeningAgent(plans()){}\nstatic kag::agent::AgentInfo info(){return {\""<<name<<"\"};}};\n}\n";
    std::ofstream(directory/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    std::ofstream manifest(directory/"agent.json");
    manifest<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"<<name<<"::Agent\",\"sources\":[\"source/agent.cpp\"";
    for(const char* agent:{"top_replay_library","public_router"})manifest<<",\""<<fs::relative(experiment/"league"/agent/"source/agent.cpp",directory).generic_string()<<"\"";
    manifest<<"]}\n";
    std::ofstream readme(directory/"README.md");
    readme<<"# "<<name<<"\n\nParent opening_router_v2, with V30-rebuilt full days from "<<fs::relative(source,experiment).generic_string()<<". "
        <<"The service and dated composition are unchanged; each selected day removes its final hire. "
        <<"C++ search adds days only when all four discovery matchups have nondecreasing mean margin and aggregate margin strictly improves. "
        <<"64 games per matchup, 1000–1031 both seats, against v2/public/Mao85/Junghoon78. "
        <<"Source55 day contracts came from seed1000 versus public_router; day0 and day18 are excluded because v2 changed their economic components. "
        <<"Full live-opponent tests verify subsequent dependencies. Selected days:";
    for(const auto& plan:plans)readme<<' '<<plan.day;
    readme<<".\n\nLocal compiler/search work over explicitly attributed parent replay/market components. "
        <<"All exact source provenance remains in the parent packages and top_replay_library IMPORT.json. "
        <<"Discovery candidate, not promoted; required checks and fresh independent gate pending. No hidden runtime inputs.\n";
}

int main(int argc,char** argv) {
    if(argc!=3) {std::cerr<<"usage: combine_days SOURCE_HIRE_RUN NEW_RUN_DIRECTORY\n";return 2;}
    const fs::path source=argv[1],directory=argv[2];const std::string run=directory.filename(),source_name=source.filename();
    for(char c:run)if(!(std::isalnum(static_cast<unsigned char>(c)) || c=='_'))return 2;
    if(run.empty() || (fs::exists(directory) && !fs::is_empty(directory)))return 2;
    fs::create_directories(directory/"exact");std::ifstream input(source/"compiled.csv");if(!input)return 2;
    std::vector<DayPlan> retained;auto incumbent=evaluate_league(retained,run+"_parent",directory/"exact");
    std::ofstream log(directory/"search.csv");log<<"proposal,day,accepted,margin_v2,margin_public,margin_mao85,margin_junghoon\n";
    std::string line;std::getline(input,line);
    while(std::getline(input,line)) {
        std::stringstream row(line);std::vector<std::string> fields;std::string field;
        while(std::getline(row,field,','))fields.push_back(field);
        if(fields.size()!=9)std::abort();
        const int id=std::stoi(fields[0]),day=std::stoi(fields[1]);
        if(fields[5]!="1" || fields[7]!="1" || fields[8]!="1" || day==0 || day==18)continue;
        const auto path=source/"proposals"/(source_name+"_"+std::to_string(id))/"combined_schedule.txt";
        auto candidate=retained;candidate.push_back(load_plan(day,path));
        const auto scores=evaluate_league(candidate,run+"_"+std::to_string(id),directory/"exact");
        bool accept=true;double improvement=0;
        for(int i=0;i<4;++i) {accept&=scores[i]>=incumbent[i];improvement+=scores[i]-incumbent[i];}
        accept&=improvement>0;
        if(accept) {retained=std::move(candidate);incumbent=scores;}
        log<<id<<','<<day<<','<<accept;for(double value:scores)log<<','<<value;log<<'\n';log.flush();
        std::cout<<"day="<<day<<" accepted="<<accept<<" aggregate_margin_change="<<improvement<<std::endl;
    }
    if(!retained.empty())export_agent(run+"_best",retained,fs::absolute("experiments/v6/sep07_compositions_v0/candidates")/(run+"_best"),source);
}
