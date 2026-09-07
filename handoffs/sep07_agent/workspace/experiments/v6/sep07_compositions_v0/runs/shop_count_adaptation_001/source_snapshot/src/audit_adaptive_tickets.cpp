#include "../include/adaptive_ticket.hpp"
#include "../include/ticket_trace.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include "../league/binghua_116/source/agent.hpp"
#include "../candidates/justin_recall_v0/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;

template<class Rival>
void evaluate(const AdaptiveAnimalPlan& plan,double buffer,int mode,const std::string& name,
              const std::string& opponent,const fs::path& directory,std::ofstream& choices) {
    Options o;o.a=name;o.b=opponent;o.validate=true;o.output=(directory/(name+"_vs_"+opponent+".json")).string();
    constexpr int count=64;std::vector<Outcome> results(count);std::array<int,count> chosen;
    std::array<std::array<double,3>,count> scores;std::atomic<int> next{0};std::vector<std::thread> workers;
    const auto start=std::chrono::steady_clock::now();
    for(int w=0;w<4;++w)workers.emplace_back([&] {
        AdaptiveTicketAgent own(150,plan,buffer,mode);Rival rival;
        for(int job=next.fetch_add(1);job<count;job=next.fetch_add(1)) {
            results[job]=run_game(own,rival,1000+job/2,job%2,o);
            chosen[job]=own.chosen();scores[job]=own.scores();
        }
    });
    for(auto& worker:workers)worker.join();
    write_results(o,results,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    for(int i=0;i<count;++i) {
        choices<<name<<','<<opponent<<','<<results[i].seed<<','<<results[i].seat<<','<<chosen[i];
        for(double score:scores[i])choices<<','<<score;
        choices<<'\n';
    }
    choices.flush();
}

void export_agent(const fs::path& directory,const std::string& name,const AdaptiveAnimalPlan& p,double buffer,int mode) {
    const auto folder=directory/"proposals"/name;fs::create_directories(folder/"source");
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    std::ofstream header(folder/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<fs::relative(experiment/"include/adaptive_ticket.hpp",folder/"source").generic_string()<<"\"\n"
        <<"namespace compositions::"<<name<<" {\nclass Agent:public AdaptiveTicketAgent {public:Agent():AdaptiveTicketAgent(150,AdaptiveAnimalPlan{AnimalEdit{"
        <<p.edit.original<<','<<p.edit.original;
    for(auto address:{p.edit.purchase,p.edit.pickup,p.edit.placement,p.edit.structure})header<<",{"<<address.step<<','<<address.index<<'}';
    header<<','<<p.edit.cell<<"},"<<p.start_day<<','<<p.end_day<<",Service{0,"<<p.service.feed<<"u,"<<p.service.care<<"u,"
        <<p.service.collect_fertilizer<<"u,"<<p.service.harvest<<"u,0}},"<<buffer<<','<<mode<<"){}\n"
        <<"static kag::agent::AgentInfo info(){return {\""<<name<<"\"};}};}\n";
    std::ofstream(folder/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    std::ofstream(folder/"agent.json")<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"
        <<name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\""<<fs::relative(experiment/"league/top_replay_library/source/agent.cpp",folder).generic_string()<<"\"]}\n";
    std::ofstream(folder/"README.md")<<"# "<<name<<"\n\nExperimental single animal choice on Justin150's terminal-improved course. "
        <<"Choose goose/cow/sheep at the first affected structure/purchase action, using only observed shops, market and public herds. "
        <<"Existing herd service/sales and future demand are approximate; new animal uses traced dated service. "
        <<"The choice preserves its purchase/pickup/place/structure closure, with existing output handling. "
        <<"No optimized day plans reused. Discovery only; deployment and fresh gates pending. Source/model lineage in IMPORT.json.\n";
    std::ofstream(folder/"IMPORT.json")<<"{\"source_program\":150,\"source_metadata\":\"league/top_replay_library/IMPORT.json\",\"terminal_parent\":\"candidates/justin_recall_v0\","
        <<"\"model_idea\":\"dmitriigluzdov/kaggriculture-goose-portfolio-historical-lb-2615 whole-herd endogenous projection\","
        <<"\"local_changes\":\"New typed explicit-service candidate biology, own or relative revenue objective, decision before earliest changed structure, per-instance observation-only choice\","
        <<"\"buffer\":"<<buffer<<",\"mode\":"<<mode<<",\"mode_definition\":\"0 projected cash,1 projected margin,2 current shop count,3 current demand units per cycle; simple modes choose cow/sheep at purchase and preserve ties\",\"status\":\"experimental, model and full deployment validation pending\"}\n";
}

int main(int argc,char** argv) {
    if(argc<2 || argc>3)return 2;const fs::path directory=fs::absolute(argv[1]);
    const bool simple=argc==3 && std::string(argv[2])=="simple";
    if(fs::exists(directory))return 2;fs::create_directories(directory/"exact");
    justin_recall_v0::Agent source;king_rc4::Agent rival;auto trace=trace_tickets_from(source,rival,1000,0);
    if(!trace.failure.empty()){std::cerr<<trace.failure<<'\n';return 3;}
    const auto market=value_market(trace.market,trace.shops);if(market.cash[0]!=trace.cash || market.cash[1]!=trace.rival_cash)std::abort();
    std::ofstream choices(directory/"choices.csv");choices<<"candidate,opponent,seed,seat,chosen,goose_score,cow_score,sheep_score\n";
    // A disabled decision checks that the wrapper preserves its terminal parent.
    const auto& first=trace.tickets[8];AdaptiveAnimalPlan identity{first.edit,first.start_day,first.end_day,{}};
    identity.service={0,first.feed,first.care,first.collect,first.harvest_requested,0};
    AdaptiveTicketAgent unchanged(150,identity,1e100,true);Options o;o.validate=true;
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat) {
        const auto a=run_game(source,rival,seed,seat,o),b=run_game(unchanged,rival,seed,seat,o);
        for(int p=0;p<2;++p)if(a.cash[p]!=b.cash[p] || a.hash[p]!=b.hash[p])std::abort();
    }
    std::ofstream(directory/"parity.json")<<"{\"disabled_choice_parent_equal_games\":16,\"baseline_both_cash_exact\":true}\n";
    Options baseline;baseline.a="parent";baseline.validate=true;baseline.threads=4;
    for(int i=0;i<32;++i)baseline.seeds.push_back(1000+i);
    for(const auto& opponent:std::array<std::string,5>{"justin_recall_v0","king_rc4","opening_router_v4","public_router","binghua_116"}) {
        baseline.b=opponent;baseline.output=(directory/"exact"/("parent_vs_"+opponent+".json")).string();
        if(opponent=="justin_recall_v0")run_batch(baseline,[]{return justin_recall_v0::Agent{};},[]{return justin_recall_v0::Agent{};});
        if(opponent=="king_rc4")run_batch(baseline,[]{return justin_recall_v0::Agent{};},[]{return king_rc4::Agent{};});
        if(opponent=="opening_router_v4")run_batch(baseline,[]{return justin_recall_v0::Agent{};},[]{return opening_router_v4::Agent{};});
        if(opponent=="public_router")run_batch(baseline,[]{return justin_recall_v0::Agent{};},[]{return public_router::Agent{};});
        if(opponent=="binghua_116")run_batch(baseline,[]{return justin_recall_v0::Agent{};},[]{return binghua_116::Agent{};});
    }
    for(int ticket:{4,5,8,9,12,13,16}) {
        const auto& t=trace.tickets[ticket];if(!t.eligible || t.edit.structure.step<0)std::abort();
        int occupants=0;for(const auto& other:trace.tickets)occupants+=other.edit.cell==t.edit.cell;
        if(occupants!=1)continue;
        AdaptiveAnimalPlan plan{t.edit,t.start_day,t.end_day,{}};
        plan.service={0,t.feed,t.care,t.collect,t.harvest_requested,0};
        for(int mode:{2,3,0,1})for(int buffer:{0,300,1000}) {
            if(simple && mode<2)continue;
            if(mode>=2 && (buffer || t.edit.original==kag::GOOSE))continue;
            const std::string name="adaptive_p150_t"+std::to_string(ticket)+"_m"+std::to_string(mode)+"_b"+std::to_string(buffer);
            export_agent(directory,name,plan,buffer,mode);
            evaluate<justin_recall_v0::Agent>(plan,buffer,mode,name,"justin_recall_v0",directory/"exact",choices);
            evaluate<king_rc4::Agent>(plan,buffer,mode,name,"king_rc4",directory/"exact",choices);
            evaluate<opening_router_v4::Agent>(plan,buffer,mode,name,"opening_router_v4",directory/"exact",choices);
            evaluate<public_router::Agent>(plan,buffer,mode,name,"public_router",directory/"exact",choices);
            evaluate<binghua_116::Agent>(plan,buffer,mode,name,"binghua_116",directory/"exact",choices);
        }
    }
}
