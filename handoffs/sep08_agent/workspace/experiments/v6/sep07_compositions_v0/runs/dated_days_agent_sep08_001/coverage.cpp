#include "../../include/evaluation.hpp"
#include "proposals/dated_days_control/source/agent.hpp"
#include "proposals/dated_days_labor/source/agent.hpp"
#include "proposals/dated_days_feed/source/agent.hpp"
#include "proposals/dated_days_combined/source/agent.hpp"
#include "../../league/public_router/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;

template<class Own,int Mode> class Observed:public Own {
    std::vector<GuardedDay> plans_;
public:
    int hours=0,unit_mismatches=0;
    Observed() {
        if constexpr(Mode==1)plans_={dated_days_data::labor_14(),dated_days_data::labor_18(),dated_days_data::labor_22(),dated_days_data::labor_26()};
        if constexpr(Mode==2)plans_={dated_days_data::feed_18(),dated_days_data::feed_26()};
        if constexpr(Mode==3)plans_={dated_days_data::combined_14(),dated_days_data::combined_18(),dated_days_data::combined_22(),dated_days_data::combined_26()};
    }
    void reset(const kag::agent::AgentInit& init){Own::reset(init);hours=unit_mismatches=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        Own::act(o,b,a);
        if(!(this->matched_days()&(uint32_t{1}<<o.day)))return;
        ++hours;
        auto p=std::find_if(plans_.begin(),plans_.end(),[&](const auto& p){return p.plan.day==o.day;});
        if(p==plans_.end())std::abort();
        unit_mismatches+=p->plan.actions[o.hour].n_units!=o.self().n_units;
    }
};

template<class Own,int Mode,class Rival> void batch(const fs::path& out,const std::string& name,const std::string& opponent) {
    Options options;options.a=name;options.b=opponent;options.validate=true;options.profile=true;
    options.output=(out/(name+"_vs_"+opponent+".json")).string();
    Observed<Own,Mode> own;Rival rival;std::vector<Outcome> games;
    std::ofstream coverage(out/(name+"_vs_"+opponent+".coverage.jsonl"));
    auto start=std::chrono::steady_clock::now();
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat) {
        games.push_back(run_game(own,rival,seed,seat,options));
        coverage<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"days\":"<<own.matched_days()
            <<",\"hours\":"<<own.hours<<",\"unit_mismatches\":"<<own.unit_mismatches<<"}\n";
    }
    write_results(options,games,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;const fs::path out=argv[1];if(fs::exists(out))return 2;fs::create_directories(out);
    batch<dated_days_control::Agent,0,public_router::Agent>(out,"dated_days_control","public_router");
    batch<dated_days_labor::Agent,1,public_router::Agent>(out,"dated_days_labor","public_router");
    batch<dated_days_feed::Agent,2,public_router::Agent>(out,"dated_days_feed","public_router");
    batch<dated_days_combined::Agent,3,public_router::Agent>(out,"dated_days_combined","public_router");
    batch<dated_days_control::Agent,0,Pass>(out,"dated_days_control","pass");
    batch<dated_days_labor::Agent,1,Pass>(out,"dated_days_labor","pass");
    batch<dated_days_feed::Agent,2,Pass>(out,"dated_days_feed","pass");
    batch<dated_days_combined::Agent,3,Pass>(out,"dated_days_combined","pass");
}
