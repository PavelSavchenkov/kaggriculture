#include "../include/evaluation.hpp"
#include "../include/deferred_animal.hpp"
#include "../runs/general_animal_entry_bank_002/bank.hpp"
#include "../candidates/investment_context_guarded_001_best/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include "../league/teammate_shoprouter/source/agent.hpp"
#include "../league/public_router_v5/source/agent.hpp"
#include <filesystem>
#include <iostream>
#include <set>

using namespace compositions;
namespace fs=std::filesystem;
using Source=investment_context_guarded_001_best::Agent;
using Policy=DeferredAnimalAgent<Source>;
struct Definition {std::string name;int day=30,item=kag::GOOSE;bool adaptive=false,relative=true;double cost=0,minimum=0;};
Policy policy(const Definition& d) {
    return {88,1,42,general_animal_entry_bank_002::entries(),d.day,d.item,
        d.adaptive?general_animal_entry_bank_002::models():std::vector<AnimalInvestmentModel>{},d.relative,d.cost,d.minimum,kag::COW};
}

template<class Rival> void evaluate(const Definition& d,const std::string& opponent,const fs::path& folder,std::ofstream& log) {
    Options o;o.a=d.name;o.b=opponent;o.validate=true;o.output=(folder/(d.name+"_vs_"+opponent+".json")).string();
    constexpr int count=64;std::vector<Outcome> results(count);
    struct Choice {int item,day,decisions,waits;std::array<double,4> scores;};std::array<Choice,count> choices;
    std::atomic<int> next{0};std::vector<std::thread> workers;const auto start=std::chrono::steady_clock::now();
    for(int worker=0;worker<4;++worker)workers.emplace_back([&] {
        auto own=policy(d);Rival rival;
        for(int i=next.fetch_add(1);i<count;i=next.fetch_add(1)) {
            results[i]=run_game(own,rival,1000+i/2,i%2,o);
            choices[i]={own.chosen(),own.entry_day(),own.decisions(),own.waits(),own.scores()};
        }
    });
    for(auto& worker:workers)worker.join();write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    int entered=0;for(int i=0;i<count;++i) {
        const auto& c=choices[i];entered+=c.item>=0;
        log<<d.name<<','<<opponent<<','<<results[i].seed<<','<<results[i].seat<<','<<c.item<<','<<c.day<<','<<c.decisions<<','<<c.waits;
        for(double score:c.scores)log<<','<<score;log<<'\n';
    }
    log.flush();std::cout<<d.name<<" vs "<<opponent<<" entered "<<entered<<'/'<<count<<std::endl;
}

void export_agent(const Definition& d,const fs::path& run) {
    const auto folder=run/"proposals"/d.name;fs::create_directories(folder/"source");
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    auto relative=[&](const fs::path& p){return fs::relative(experiment/p,folder/"source").generic_string();};
    std::ofstream h(folder/"source/agent.hpp");
    h<<"#pragma once\n#include \""<<relative("include/deferred_animal.hpp")<<"\"\n#include \""<<relative("runs/general_animal_entry_bank_002/bank.hpp")<<"\"\n#include \""<<relative("candidates/investment_context_guarded_001_best/source/agent.hpp")<<"\"\nnamespace compositions::"<<d.name<<" {using Base=DeferredAnimalAgent<investment_context_guarded_001_best::Agent>;class Agent:public Base{public:Agent():Base(88,1,42,general_animal_entry_bank_002::entries(),"<<d.day<<','<<d.item<<','<<(d.adaptive?"general_animal_entry_bank_002::models()":"std::vector<AnimalInvestmentModel>{}")<<','<<d.relative<<','<<d.cost<<','<<d.minimum<<",10){}static kag::agent::AgentInfo info(){return {\""<<d.name<<"\"};}};}\n";
    std::ofstream(folder/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    std::ofstream(folder/"agent.json")<<"{\"format_version\":1,\"name\":\""<<d.name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"<<d.name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<fs::relative(experiment/"league/top_replay_library/source/agent.cpp",folder).generic_string()<<"\"]}\n";
    std::ofstream(folder/"IMPORT.json")<<"{\"parent\":\"candidates/investment_context_guarded_001_best\",\"entry_bank\":\"runs/general_animal_entry_bank_002/LINEAGE.json\",\"idea\":\"User: general cow/sheep/goose/wait choice using already observed shops; Dmitrii Gluzdov whole-herd price-impact projection\",\"local_changes\":\"Typed daily whole-farm intended trade projection, feed/fertilizer/service accounting, receding-horizon purchase dates, exact V30 entry days with preserved crop release\",\"adaptive\":"<<d.adaptive<<",\"relative\":"<<d.relative<<",\"operation_cost\":"<<d.cost<<",\"minimum_gain\":"<<d.minimum<<",\"status\":\"Discovery only; full-game realization and promotion pending\"}\n";
    std::ofstream(folder/"README.md")<<"# "<<d.name<<"\n\nExperimental animal investment on the validated shop-adaptive parent. Choices include all3species and keeping the investment open for a later compiled entry day. Physical entry guards, explicit structures/placement/feed/care, and mixed-product deposits/sales. The old future service calendar is reused and must be checked under exact replay. Price, rival expansion, storage and marginal labor remain heuristic. Lineage in IMPORT.json; no promotion yet.\n";
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;const fs::path folder=fs::absolute(argv[1]);if(fs::exists(folder))return 2;fs::create_directories(folder/"exact");
    Definition parent{"early2_animal_parent",-1};Source source;public_router::Agent rival;auto control=policy(parent);Options o;o.validate=true;
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat){const auto a=run_game(source,rival,seed,seat,o),b=run_game(control,rival,seed,seat,o);for(int p=0;p<2;++p)if(a.cash[p]!=b.cash[p] || a.hash[p]!=b.hash[p])std::abort();}
    std::ofstream(folder/"parity.json")<<"{\"disabled_parent_cash_and_actions_equal\":16}\n";
    std::vector<Definition> definitions{parent,{"early2_animal_wait",30}};std::set<std::pair<int,int>> fixed;
    for(const auto& e:general_animal_entry_bank_002::entries())fixed.insert({e.day.plan.day,e.item});
    for(const auto [day,item]:fixed)definitions.push_back({"early2_animal_d"+std::to_string(day)+"_i"+std::to_string(item),day,item});
    for(int relative:{0,1})for(int cost:{0,3,8})for(int minimum:{0,500})
        definitions.push_back({"early2_animal_adaptive_r"+std::to_string(relative)+"_c"+std::to_string(cost)+"_b"+std::to_string(minimum),30,kag::GOOSE,true,bool(relative),double(cost),double(minimum)});
    std::ofstream log(folder/"choices.csv");log<<"candidate,opponent,seed,seat,item,entry_day,decisions,waits,goose,cow,sheep,wait\n";
    for(const auto& d:definitions) {
        export_agent(d,folder);
        evaluate<Source>(d,"investment_context_guarded_001_best",folder/"exact",log);
        evaluate<public_router::Agent>(d,"public_router",folder/"exact",log);
        evaluate<king_rc4::Agent>(d,"king_rc4",folder/"exact",log);
        evaluate<opening_router_v4::Agent>(d,"opening_router_v4",folder/"exact",log);
        evaluate<teammate_shoprouter::Agent>(d,"teammate_shoprouter",folder/"exact",log);
        evaluate<public_router_v5::Agent>(d,"public_router_v5",folder/"exact",log);
    }
}
