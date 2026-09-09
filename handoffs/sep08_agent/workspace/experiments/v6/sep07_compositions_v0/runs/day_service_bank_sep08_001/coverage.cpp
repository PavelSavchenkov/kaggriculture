#include "../../include/evaluation.hpp"
#include <filesystem>
#include "proposals/service_bank_p362_m0/source/agent.hpp"
#include "proposals/service_bank_p362_m1/source/agent.hpp"
#include "proposals/service_bank_p362_m2/source/agent.hpp"
#include "proposals/service_bank_p362_m3/source/agent.hpp"
#include "../../league/public_router/source/agent.hpp"
#include "../empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2/source/agent.hpp"
using namespace compositions;namespace fs=std::filesystem;
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
    for(int seed=1000;seed<1016;++seed)for(int seat=0;seat<2;++seat) {
        games.push_back(run_game(own,rival,seed,seat,options));
        coverage<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"days\":"<<own.matched_days()
            <<",\"max_act_ms\":"<<own.max_act_ms<<",\"total_act_ms\":"<<own.total_act_ms<<",\"daily\":[";
        for(int d=0;d<30;++d){if(d)coverage<<',';coverage<<'[';for(int i=0;i<5;++i){if(i)coverage<<',';coverage<<own.daily[d][i];}coverage<<']';}
        coverage<<"]}\n";
    }
    write_results(options,games,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
}

int main(int argc,char** argv){if(argc!=2)return 2;fs::path out=argv[1];if(fs::exists(out))return 2;fs::create_directories(out);
batch<service_bank_p362_m0::Agent,empty_sale_slots_m2::Agent>(out,"service_bank_p362_m0","empty_sale_slots_m2");
batch<service_bank_p362_m0::Agent,public_router::Agent>(out,"service_bank_p362_m0","public_router");
batch<service_bank_p362_m0::Agent,joint_routes_p362_m0::Agent>(out,"service_bank_p362_m0","joint_routes_p362_m0");
batch<service_bank_p362_m0::Agent,day_program_p362_m3::Agent>(out,"service_bank_p362_m0","day_program_p362_m3");
batch<service_bank_p362_m0::Agent,Pass>(out,"service_bank_p362_m0","pass");
batch<service_bank_p362_m1::Agent,empty_sale_slots_m2::Agent>(out,"service_bank_p362_m1","empty_sale_slots_m2");
batch<service_bank_p362_m1::Agent,public_router::Agent>(out,"service_bank_p362_m1","public_router");
batch<service_bank_p362_m1::Agent,joint_routes_p362_m0::Agent>(out,"service_bank_p362_m1","joint_routes_p362_m0");
batch<service_bank_p362_m1::Agent,day_program_p362_m3::Agent>(out,"service_bank_p362_m1","day_program_p362_m3");
batch<service_bank_p362_m1::Agent,Pass>(out,"service_bank_p362_m1","pass");
batch<service_bank_p362_m2::Agent,empty_sale_slots_m2::Agent>(out,"service_bank_p362_m2","empty_sale_slots_m2");
batch<service_bank_p362_m2::Agent,public_router::Agent>(out,"service_bank_p362_m2","public_router");
batch<service_bank_p362_m2::Agent,joint_routes_p362_m0::Agent>(out,"service_bank_p362_m2","joint_routes_p362_m0");
batch<service_bank_p362_m2::Agent,day_program_p362_m3::Agent>(out,"service_bank_p362_m2","day_program_p362_m3");
batch<service_bank_p362_m2::Agent,Pass>(out,"service_bank_p362_m2","pass");
batch<service_bank_p362_m3::Agent,empty_sale_slots_m2::Agent>(out,"service_bank_p362_m3","empty_sale_slots_m2");
batch<service_bank_p362_m3::Agent,public_router::Agent>(out,"service_bank_p362_m3","public_router");
batch<service_bank_p362_m3::Agent,joint_routes_p362_m0::Agent>(out,"service_bank_p362_m3","joint_routes_p362_m0");
batch<service_bank_p362_m3::Agent,day_program_p362_m3::Agent>(out,"service_bank_p362_m3","day_program_p362_m3");
batch<service_bank_p362_m3::Agent,Pass>(out,"service_bank_p362_m3","pass");
}
