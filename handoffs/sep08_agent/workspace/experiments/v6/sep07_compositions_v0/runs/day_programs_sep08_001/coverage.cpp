#include "../../include/evaluation.hpp"
#include "proposals/day_program_p355_m0/source/agent.hpp"
#include "proposals/day_program_p355_m1/source/agent.hpp"
#include "proposals/day_program_p355_m2/source/agent.hpp"
#include "proposals/day_program_p355_m3/source/agent.hpp"
#include "proposals/day_program_p362_m0/source/agent.hpp"
#include "proposals/day_program_p362_m1/source/agent.hpp"
#include "proposals/day_program_p362_m2/source/agent.hpp"
#include "proposals/day_program_p362_m3/source/agent.hpp"
#include "../../league/public_router/source/agent.hpp"
#include "../observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;

template<class Own> class Observed:public Own {
public:
    std::array<std::array<int,5>,30> daily{};
    double max_act_ms=0,total_act_ms=0;
    void reset(const kag::agent::AgentInit& init){Own::reset(init);daily={};max_act_ms=total_act_ms=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        const std::array<int,5> before={this->active_hours(),this->abandoned_days(),this->screened_programs(),this->rejected_programs(),this->skipped_actions()};
        auto start=std::chrono::steady_clock::now();Own::act(o,b,a);
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        total_act_ms+=ms;max_act_ms=std::max(max_act_ms,ms);
        const std::array<int,5> after={this->active_hours(),this->abandoned_days(),this->screened_programs(),this->rejected_programs(),this->skipped_actions()};
        for(int i=0;i<5;++i)daily[o.day][i]+=after[i]-before[i];
    }
};

template<class Own,class Rival> void batch(const fs::path& out,const std::string& name,const std::string& opponent) {
    Options options;options.a=name;options.b=opponent;options.validate=true;options.profile=true;options.expansions=100000;
    options.output=(out/(name+"_vs_"+opponent+".json")).string();
    Observed<Own> own;Rival rival;std::vector<Outcome> games;
    std::ofstream coverage(out/(name+"_vs_"+opponent+".coverage.jsonl"));
    auto start=std::chrono::steady_clock::now();
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat) {
        games.push_back(run_game(own,rival,seed,seat,options));
        coverage<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"days\":"<<own.matched_days()
            <<",\"max_act_ms\":"<<own.max_act_ms<<",\"total_act_ms\":"<<own.total_act_ms<<",\"daily\":[";
        for(int d=0;d<30;++d){if(d)coverage<<',';coverage<<'[';for(int i=0;i<5;++i){if(i)coverage<<',';coverage<<own.daily[d][i];}coverage<<']';}
        coverage<<"]}\n";
    }
    write_results(options,games,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
}

template<class Own,class Parent> void opponents(const fs::path& out,const std::string& name,const std::string& parent) {
    batch<Own,public_router::Agent>(out,name,"public_router");
    batch<Own,kag::agents::observed_sale_lead_start_216::Agent>(out,name,"observed_sale_lead_start_216");
    batch<Own,Parent>(out,name,parent);
    batch<Own,Pass>(out,name,"pass");
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;const fs::path out=argv[1];if(fs::exists(out))return 2;fs::create_directories(out);
    opponents<day_program_p355_m0::Agent,joint_routes_p355_m0::Agent>(out,"day_program_p355_m0","joint_routes_p355_m0");
    opponents<day_program_p355_m1::Agent,joint_routes_p355_m0::Agent>(out,"day_program_p355_m1","joint_routes_p355_m0");
    opponents<day_program_p355_m2::Agent,joint_routes_p355_m0::Agent>(out,"day_program_p355_m2","joint_routes_p355_m0");
    opponents<day_program_p355_m3::Agent,joint_routes_p355_m0::Agent>(out,"day_program_p355_m3","joint_routes_p355_m0");
    opponents<day_program_p362_m0::Agent,joint_routes_p362_m0::Agent>(out,"day_program_p362_m0","joint_routes_p362_m0");
    opponents<day_program_p362_m1::Agent,joint_routes_p362_m0::Agent>(out,"day_program_p362_m1","joint_routes_p362_m0");
    opponents<day_program_p362_m2::Agent,joint_routes_p362_m0::Agent>(out,"day_program_p362_m2","joint_routes_p362_m0");
    opponents<day_program_p362_m3::Agent,joint_routes_p362_m0::Agent>(out,"day_program_p362_m3","joint_routes_p362_m0");
}
