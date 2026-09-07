#include "../include/shop_herd.hpp"
#include "../include/ticket_trace.hpp"
#include "../candidates/justin_guarded_hires_001_best/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include "../league/binghua_116/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

template<class Rival>
void evaluate(const std::vector<AdaptiveAnimalPlan>& plans,int mode,bool guarded,const std::string& name,
              const std::string& opponent,const fs::path& directory,std::ofstream& choices) {
    Options o;o.a=name;o.b=opponent;o.validate=true;o.output=(directory/(name+"_vs_"+opponent+".json")).string();
    constexpr int count=64;std::vector<Outcome> results(count);std::array<std::vector<int>,count> selected;
    std::array<uint32_t,count> days{};std::atomic<int> next{0};std::vector<std::thread> workers;
    const auto start=std::chrono::steady_clock::now();
    for(int w=0;w<4;++w)workers.emplace_back([&] {
        ShopHerdAgent own(plans,mode,guarded);Rival rival;
        for(int job=next.fetch_add(1);job<count;job=next.fetch_add(1)) {
            results[job]=run_game(own,rival,1000+job/2,job%2,o);
            selected[job]=own.chosen();days[job]=own.matched_days();
        }
    });
    for(auto& worker:workers)worker.join();
    write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    for(int i=0;i<count;++i) {
        choices<<name<<','<<opponent<<','<<results[i].seed<<','<<results[i].seat<<','<<days[i]<<',';
        for(int j=0;j<int(selected[i].size());++j)choices<<(j?";":"")<<selected[i][j];
        choices<<'\n';
    }
    choices.flush();
}

void export_agent(const fs::path& directory,const std::string& name,const std::vector<AdaptiveAnimalPlan>& plans,int mode,bool guarded) {
    const auto folder=directory/"proposals"/name;fs::create_directories(folder/"source");
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    std::ofstream header(folder/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<fs::relative(experiment/"include/shop_herd.hpp",folder/"source").generic_string()<<"\"\n"
        <<"namespace compositions::"<<name<<" {\nclass Agent:public ShopHerdAgent {public:Agent():ShopHerdAgent({";
    for(int i=0;i<int(plans.size());++i) {
        const auto& p=plans[i];header<<(i?",":"")<<"AdaptiveAnimalPlan{AnimalEdit{"<<p.edit.original<<','<<p.edit.original;
        for(auto address:{p.edit.purchase,p.edit.pickup,p.edit.placement,p.edit.structure})header<<",{"<<address.step<<','<<address.index<<'}';
        header<<','<<p.edit.cell<<"},"<<p.start_day<<','<<p.end_day<<",Service{0,"<<p.service.feed<<"u,"<<p.service.care<<"u,"
            <<p.service.collect_fertilizer<<"u,"<<p.service.harvest<<"u,0}}";
    }
    header<<"},"<<mode<<','<<guarded<<"){}\nstatic kag::agent::AgentInfo info(){return {\""<<name<<"\"};}};}\n";
    std::ofstream(folder/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    std::ofstream(folder/"agent.json")<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"
        <<name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<fs::relative(experiment/"league/top_replay_library/source/agent.cpp",folder).generic_string()<<"\"]}\n";
    std::ofstream(folder/"README.md")<<"# "<<name<<"\n\nExperimental shop-conditioned purchases on Justin150's terminal-improved course. "
        <<"Choose cows or sheep from currently observed milk/wool shop demand; preserve the original animal on ties. "
        <<"Purchase, pickup, placement and output handling are changed together. Reuse of full day plans requires their original physical starting state "
        <<"and excludes changed purchase/transfer days. Changed herds therefore lose some existing labor savings until their days are rebuilt. "
        <<"The user's shop adaptation intuition is the policy source; source replay and local component lineage are in IMPORT.json. "
        <<"Discovery only; fresh and deployment checks pending.\n";
    std::ofstream(folder/"IMPORT.json")<<"{\"source_program\":150,\"source_metadata\":\"league/top_replay_library/IMPORT.json\","
        <<"\"terminal_parent\":\"candidates/justin_recall_v0\",\"shop_choice_idea\":\"User: more milk-demand shops means more cows; more wool-demand shops means more sheep\","
        <<"\"local_changes\":\"Independent dated purchase choices, exact addressed purchase/transfer closure, mixed-product deposits and sales, physically guarded V30 reuse\","
        <<"\"mode\":"<<mode<<",\"mode_definition\":\"2 observed shop count; 3 observed demand units per cycle (Yarn counts twice); ties keep the original animal\","
        <<"\"guarded_days\":"<<(guarded?"true":"false")<<",\"status\":\"discovery only\"}\n";
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;const fs::path directory=fs::absolute(argv[1]);
    if(fs::exists(directory))return 2;fs::create_directories(directory/"exact");
    justin_recall_v0::Agent source;king_rc4::Agent rival;auto trace=trace_tickets_from(source,rival,1000,0);
    if(!trace.failure.empty())return 3;
    std::array<AdaptiveAnimalPlan,3> available;
    int index=0;for(int ticket:{5,8,9}) {
        const auto& t=trace.tickets[ticket];if(!t.eligible)std::abort();
        available[index++]={t.edit,t.start_day,t.end_day,{0,t.feed,t.care,t.collect,t.harvest_requested,0}};
    }
    Options o;o.validate=true;
    ShopHerdAgent identity({},3,true);justin_guarded_hires_001_best::Agent best;
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat) {
        const auto a=run_game(identity,rival,seed,seat,o),b=run_game(best,rival,seed,seat,o);
        for(int p=0;p<2;++p)if(a.cash[p]!=b.cash[p] || a.hash[p]!=b.hash[p])std::abort();
    }
    for(const auto& plan:available)for(int mode:{2,3}) {
        ShopHerdAgent combined({plan},mode,false);AdaptiveTicketAgent original(150,plan,0,mode);
        for(int seed=1000;seed<1032;++seed)for(int seat=0;seat<2;++seat) {
            const auto a=run_game(combined,rival,seed,seat,o),b=run_game(original,rival,seed,seat,o);
            for(int p=0;p<2;++p)if(a.cash[p]!=b.cash[p] || a.hash[p]!=b.hash[p])std::abort();
        }
    }
    std::ofstream(directory/"parity.json")<<"{\"empty_guarded_choice_best_equal_games\":16,\"single_choice_original_equal_games\":384}\n";
    std::ofstream choices(directory/"choices.csv");choices<<"candidate,opponent,seed,seat,matched_day_mask,chosen\n";
    for(int mask=0;mask<8;++mask)for(int mode:{2,3})for(bool guarded:{false,true}) {
        if(mask==0 && mode==2)continue;
        std::vector<AdaptiveAnimalPlan> plans;for(int i=0;i<3;++i)if(mask&(1<<i))plans.push_back(available[i]);
        const std::string name="shop_herd_s"+std::to_string(mask)+"_m"+std::to_string(mode)+"_g"+std::to_string(guarded);
        export_agent(directory,name,plans,mode,guarded);
        evaluate<justin_guarded_hires_001_best::Agent>(plans,mode,guarded,name,"justin_guarded_hires_001_best",directory/"exact",choices);
        evaluate<justin_recall_v0::Agent>(plans,mode,guarded,name,"justin_recall_v0",directory/"exact",choices);
        evaluate<king_rc4::Agent>(plans,mode,guarded,name,"king_rc4",directory/"exact",choices);
        evaluate<opening_router_v4::Agent>(plans,mode,guarded,name,"opening_router_v4",directory/"exact",choices);
        evaluate<public_router::Agent>(plans,mode,guarded,name,"public_router",directory/"exact",choices);
        evaluate<binghua_116::Agent>(plans,mode,guarded,name,"binghua_116",directory/"exact",choices);
    }
}
