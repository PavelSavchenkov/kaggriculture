#include "../include/evaluation.hpp"
#include "../include/deferred_animal.hpp"
#include "../runs/general_animal_entry_bank_002/bank.hpp"
#include "../runs/animal_entry_bank_001/bank.hpp"
#include "../runs/investment_day_contexts_001/days.hpp"
#include "../candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../league/public_router_v5/source/agent.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include "../league/teammate_shoprouter/source/agent.hpp"
#include "../runs/atakan_portfolio_001/proposals/atakan_demand/source/agent.hpp"
#include "../runs/atakan_portfolio_001/proposals/atakan_value_margin/source/agent.hpp"
#include <filesystem>
#include <iostream>
#include <variant>

using namespace compositions;
namespace fs=std::filesystem;
using Source=investment_context_guarded_001_best::Agent;
using Early=DeferredAnimalAgent<Source>;
using LateBase=DeferredAnimalAgent<shop_herd_guarded_001_best::Agent>;
using Late=GuardedDayAgent<LateBase>;
constexpr const char* selected_days="13,14,15,16,18,19,20,23,24,25,27,113,114,115,116,118,119,120,122,123,124,125,127";
auto late_days(){return investment_context_days::select({13,14,15,16,18,19,20,23,24,25,27,113,114,115,116,118,119,120,122,123,124,125,127});}
struct Definition {std::string name;bool early=false,relative=true;int samples=0;};
struct Policy {
    std::variant<Early,Late> value;
    explicit Policy(const Definition& d):value(std::in_place_type<Early>,88,1,42) {
        if(d.early)value.emplace<Early>(88,1,42,general_animal_entry_bank_002::entries(),30,kag::GOOSE,
            general_animal_entry_bank_002::models(),d.relative,0,0,kag::COW,d.samples);
        else value.emplace<Late>(late_days(),LateBase(265,7,32,animal_entry_bank_001::entries(),30,kag::GOOSE,
            animal_entry_bank_001::models(),d.relative,0,0,kag::GOOSE,d.samples));
    }
    void reset(const kag::agent::AgentInit& i){std::visit([&](auto& a){a.reset(i);},value);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){std::visit([&](auto& x){x.act(o,b,a);},value);}
    template<class F> auto inspect(F f) const {
        if(value.index()==0)return f(std::get<Early>(value));
        return f(std::get<Late>(value).base_agent());
    }
};

template<class Rival> void evaluate(const Definition& d,const std::string& opponent,const fs::path& folder,std::ofstream& log) {
    Options options;options.a=d.name;options.b=opponent;options.validate=true;
    options.output=(folder/(d.name+"_vs_"+opponent+".json")).string();
    constexpr int count=64;std::vector<Outcome> results(count);
    struct Choice {int item,day,decisions,waits;std::array<double,4> scores;};std::array<Choice,count> choices;
    std::atomic<int> next{0};std::vector<std::thread> workers;const auto start=std::chrono::steady_clock::now();
    for(int w=0;w<4;++w)workers.emplace_back([&]{
        Policy own(d);Rival rival;
        for(int i=next.fetch_add(1);i<count;i=next.fetch_add(1)) {
            results[i]=run_game(own,rival,1000+i/2,i%2,options);
            choices[i]=own.inspect([](const auto& a){return Choice{a.chosen(),a.entry_day(),a.decisions(),a.waits(),a.scores()};});
        }
    });
    for(auto& worker:workers)worker.join();
    write_results(options,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    for(int i=0;i<count;++i) {
        const auto& c=choices[i];log<<d.name<<','<<opponent<<','<<results[i].seed<<','<<results[i].seat<<','<<c.item<<','<<c.day<<','<<c.decisions<<','<<c.waits;
        for(double score:c.scores)log<<','<<score;log<<'\n';
    }
    log.flush();
}

void export_agent(const Definition& d,const fs::path& run) {
    const auto folder=run/"proposals"/d.name;fs::create_directories(folder/"source");
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    auto relative=[&](const fs::path& p){return fs::relative(experiment/p,folder/"source").generic_string();};
    const std::string bank=d.early?"general_animal_entry_bank_002":"animal_entry_bank_001";
    const std::string parent=d.early?"investment_context_guarded_001_best":"shop_herd_guarded_001_best";
    std::ofstream h(folder/"source/agent.hpp");
    h<<"#pragma once\n#include \""<<relative("include/deferred_animal.hpp")<<"\"\n#include \""<<relative("runs/"+bank+"/bank.hpp")<<"\"\n#include \""<<relative("candidates/"+parent+"/source/agent.hpp")<<"\"\n";
    if(!d.early)h<<"#include \""<<relative("runs/investment_day_contexts_001/days.hpp")<<"\"\n";
    h<<"namespace compositions::"<<d.name<<" {using Base=DeferredAnimalAgent<"<<parent<<"::Agent>;class Agent:public "<<(d.early?"Base":"GuardedDayAgent<Base>")<<" {public:Agent():";
    if(d.early)h<<"Base(";
    else h<<"GuardedDayAgent<Base>(investment_context_days::select({"<<selected_days<<"}),Base(";
    h<<(d.early?"88,1,42,":"265,7,32,")<<bank<<"::entries(),30,9,"<<bank<<"::models(),"<<d.relative<<",0,0,"<<(d.early?10:9)<<','<<d.samples<<')';
    if(!d.early)h<<')';h<<"{} static kag::agent::AgentInfo info(){return {\""<<d.name<<"\"};}};}\n";
    std::ofstream(folder/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    std::ofstream(folder/"agent.json")<<"{\"format_version\":1,\"name\":\""<<d.name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"<<d.name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<fs::relative(experiment/"league/top_replay_library/source/agent.cpp",folder).generic_string()<<"\"]}\n";
    std::ofstream(folder/"IMPORT.json")<<"{\"parent\":\"candidates/"<<parent<<"\",\"entry_lineage\":\"runs/"<<bank<<"/LINEAGE.json\",\"worker_lineage\":\"runs/investment_day_contexts_001/LINEAGE.json\",\"local_idea\":\"User composition/value/compile feedback; sample future shop uncertainty before applying nonlinear shared market prices\",\"samples\":"<<d.samples<<",\"relative\":"<<d.relative<<",\"status\":\"Discovery; no promotion\"}\n";
    std::ofstream(folder/"README.md")<<"# "<<d.name<<"\n\nAnimal choice using observed shops, own compiled flow plans and public rival herds. Model0 preserves the previous discounted-demand approximation; model1 uses undiscounted mean demand; models8/32/64 integrate possible unknown shop sequences through the price curve. Future samples are independent of the environment seed. These remain heuristic forecasts. Exact entry plans and source service calendars are retained; late variants retain23 physical worker-day guards. Full-game validation and timing required before promotion. Source lineage in IMPORT.json.\n";
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;const auto folder=fs::absolute(argv[1]);if(fs::exists(folder))return 2;fs::create_directories(folder/"exact");
    Definition control{"sample_late_m0_r1",false,true,0};Policy original(control);Source source;public_router::Agent rival;Options options;options.validate=true;
    for(int seed=1000;seed<1032;++seed)for(int seat=0;seat<2;++seat) {
        const auto a=run_game(original,rival,seed,seat,options),b=run_game(source,rival,seed,seat,options);
        for(int p=0;p<2;++p)if(a.cash[p]!=b.cash[p] || a.hash[p]!=b.hash[p])std::abort();
    }
    std::ofstream(folder/"parity.json")<<"{\"late_zero_model_parent_cash_and_actions_equal\":64}\n";
    std::ofstream log(folder/"choices.csv");log<<"candidate,opponent,seed,seat,item,entry_day,decisions,waits,goose,cow,sheep,wait\n";
    for(bool early:{false,true})for(int samples:{0,1,8,32,64})for(bool relative:{false,true}) {
        Definition d{"sample_"+std::string(early?"early":"late")+"_m"+std::to_string(samples)+"_r"+std::to_string(relative),early,relative,samples};
        export_agent(d,folder);
        evaluate<Source>(d,"investment_context_guarded_001_best",folder/"exact",log);
        evaluate<public_router::Agent>(d,"public_router",folder/"exact",log);
        evaluate<king_rc4::Agent>(d,"king_rc4",folder/"exact",log);
        evaluate<opening_router_v4::Agent>(d,"opening_router_v4",folder/"exact",log);
        evaluate<teammate_shoprouter::Agent>(d,"teammate_shoprouter",folder/"exact",log);
        evaluate<public_router_v5::Agent>(d,"public_router_v5",folder/"exact",log);
        evaluate<atakan_demand::Agent>(d,"atakan_demand",folder/"exact",log);
        evaluate<atakan_value_margin::Agent>(d,"atakan_value_margin",folder/"exact",log);
    }
}
