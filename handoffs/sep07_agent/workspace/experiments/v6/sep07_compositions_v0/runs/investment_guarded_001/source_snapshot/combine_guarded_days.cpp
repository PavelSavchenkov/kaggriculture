#include "../include/evaluation.hpp"
#include "../candidates/justin_recall_v0/source/agent.hpp"
#include "../candidates/opening_router_v4/source/agent.hpp"
#include "../league/king_rc4/source/agent.hpp"
#include "../league/binghua_116/source/agent.hpp"
#include "../league/junghoon_78/source/agent.hpp"
#ifdef SHOP_HERD_DAYS
#include "../runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp"
#elif defined(INVESTMENT_DAYS)
#include "../runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp"
#endif
#include "days.hpp"
#include <filesystem>
#include <iostream>

using namespace compositions;
namespace fs=std::filesystem;
#ifdef SHOP_HERD_DAYS
using Base=shop_herd_s6_m3_g1::Agent;
namespace SelectedLibrary=shop_herd_days;
constexpr const char* BaseName="shop_herd_s6_m3_g1";
constexpr const char* BaseType="shop_herd_s6_m3_g1::Agent";
constexpr const char* BaseHeader="runs/shop_herd_combinations_001/proposals/shop_herd_s6_m3_g1/source/agent.hpp";
constexpr const char* LibraryName="shop_herd_days";
#elif defined(INVESTMENT_DAYS)
using Base=animal_adaptive_r1_c0_b0::Agent;
namespace SelectedLibrary=investment_days;
constexpr const char* BaseName="animal_adaptive_r1_c0_b0";
constexpr const char* BaseType="animal_adaptive_r1_c0_b0::Agent";
constexpr const char* BaseHeader="runs/animal_investments_001/proposals/animal_adaptive_r1_c0_b0/source/agent.hpp";
constexpr const char* LibraryName="investment_days";
#else
using Base=justin_recall_v0::Agent;
namespace SelectedLibrary=day_library;
constexpr const char* BaseName="justin_recall_v0";
constexpr const char* BaseType="justin_recall_v0::Agent";
constexpr const char* BaseHeader="candidates/justin_recall_v0/source/agent.hpp";
constexpr const char* LibraryName="day_library";
#endif
struct Score {double margin=0,utility=0;};

template<class Rival>
Score evaluate(const std::vector<int>& ids,const std::string& name,const std::string& opponent,
               const fs::path& directory,std::ofstream& activations) {
    Options o;o.a=name;o.b=opponent;o.validate=true;o.output=(directory/(name+"_vs_"+opponent+".json")).string();
    GuardedDayAgent<Base> own(SelectedLibrary::select(ids));Rival rival;
    std::vector<Outcome> games;Score score;
    const auto start=std::chrono::steady_clock::now();
    for(int seed=1000;seed<1016;++seed)for(int seat=0;seat<2;++seat) {
        auto g=run_game(own,rival,seed,seat,o);const double margin=g.cash[seat]-g.cash[1-seat];
        score.margin+=margin;score.utility+=(margin>0)+.5*(margin==0);
        activations<<name<<','<<opponent<<','<<seed<<','<<seat<<','<<own.matched_days()<<'\n';
        games.push_back(std::move(g));
    }
    score.margin/=games.size();score.utility/=games.size();
    write_results(o,games,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
    return score;
}

std::array<Score,6> league(const std::vector<int>& ids,const std::string& name,const fs::path& directory,std::ofstream& activations) {
    return {evaluate<Base>(ids,name,BaseName,directory,activations),
        evaluate<opening_router_v4::Agent>(ids,name,"opening_router_v4",directory,activations),
        evaluate<king_rc4::Agent>(ids,name,"king_rc4",directory,activations),
        evaluate<binghua_116::Agent>(ids,name,"binghua_116",directory,activations),
        evaluate<public_router::Agent>(ids,name,"public_router",directory,activations),
        evaluate<junghoon_78::Agent>(ids,name,"junghoon_78",directory,activations)};
}

void export_agent(const std::string& name,const std::vector<int>& ids,const fs::path& experiment,const fs::path& library) {
    const auto folder=experiment/"candidates"/name;if(fs::exists(folder))std::abort();
    fs::create_directories(folder/"source");std::ofstream header(folder/"source/agent.hpp");
    header<<"#pragma once\n#include \""<<fs::relative(experiment/BaseHeader,folder/"source").generic_string()<<"\"\n#include \""
        <<fs::relative(library/"days.hpp",folder/"source").generic_string()<<"\"\nnamespace compositions::"<<name<<" {\n"
        <<"class Agent:public GuardedDayAgent<"<<BaseType<<"> {public:Agent():GuardedDayAgent<"<<BaseType<<">("<<LibraryName<<"::select({";
    for(int i=0;i<int(ids.size());++i)header<<(i?",":"")<<ids[i];
    header<<"})){}\nstatic kag::agent::AgentInfo info(){return {\""<<name<<"\"};}};\n}\n";
    std::ofstream(folder/"source/agent.cpp")<<"#include \"agent.hpp\"\n";
    std::ofstream manifest(folder/"agent.json");
    manifest<<"{\"format_version\":1,\"name\":\""<<name<<"\",\"header\":\"source/agent.hpp\",\"type\":\"compositions::"
        <<name<<"::Agent\",\"sources\":[\"source/agent.cpp\",\"../../league/top_replay_library/source/agent.cpp\"]}\n";
    std::ofstream(folder/"README.md")<<"# "<<name<<"\n\nParent "<<BaseName<<" with complete V30 days selected by C++ league search. "
        <<"A physical day-start mismatch retains the parent day. Source services, dated composition and economic order slots are preserved; "
        <<"one hire is removed per compiled day. Future finance is not certified by the guard. "
        <<"Discovery only:32 games per opponent against parent/v4/King/Binghua/public/Jun, seeds1000..1015 both seats. "
        <<"Every accepted addition has nondecreasing mean margin and win utility in all six matchups, with strictly positive aggregate margin gain. "
        <<"Required deployment, broader and fresh checks pending. Exact local/source lineage in IMPORT.json. No hidden runtime inputs.\n";
}

int main(int argc,char** argv) {
    if(argc!=3) {std::cerr<<"usage: combine_guarded_days NEW_RUN LIBRARY_DIRECTORY\n";return 2;}
    const fs::path directory=fs::absolute(argv[1]),library=fs::absolute(argv[2]);
    const auto experiment=fs::absolute("experiments/v6/sep07_compositions_v0");
    const std::string run=directory.filename();
    for(char c:run)if(!(std::isalnum(static_cast<unsigned char>(c)) || c=='_'))return 2;
    if(run.empty() || fs::exists(directory))return 2;fs::create_directories(directory/"exact");
    std::ofstream activations(directory/"activations.csv");activations<<"candidate,opponent,seed,seat,matched_day_mask\n";
    std::ofstream search(directory/"search.csv");search<<"id,day,accepted,aggregate_margin_change";
    for(const char* name:{"parent","v4","king","binghua","public","jun"})search<<",margin_"<<name<<",utility_"<<name;
    search<<'\n';std::vector<int> selected;
    auto incumbent=league(selected,run+"_parent",directory/"exact",activations);
    for(const auto& entry:SelectedLibrary::entries()) {
        auto candidate=selected;candidate.push_back(entry.id);
        const auto scores=league(candidate,run+"_"+std::to_string(entry.id),directory/"exact",activations);
        bool accept=true;double gain=0;
        for(int i=0;i<6;++i) {
            accept&=scores[i].margin>=incumbent[i].margin && scores[i].utility>=incumbent[i].utility;
            gain+=scores[i].margin-incumbent[i].margin;
        }
        accept&=gain>0;
        if(accept) {selected=std::move(candidate);incumbent=scores;}
        search<<entry.id<<','<<entry.day.plan.day<<','<<accept<<','<<gain;
        for(const auto& s:scores)search<<','<<s.margin<<','<<s.utility;
        search<<'\n';search.flush();activations.flush();
        std::cout<<"day="<<entry.day.plan.day<<" accepted="<<accept<<" aggregate_margin_change="<<gain<<std::endl;
    }
    std::ofstream selection(directory/"selection.json");selection<<"{\"ids\":[";
    for(int i=0;i<int(selected.size());++i)selection<<(i?",":"")<<selected[i];
    selection<<"],\"agent\":\""<<run<<"_best\"}\n";
    if(!selected.empty())export_agent(run+"_best",selected,experiment,library);
}
