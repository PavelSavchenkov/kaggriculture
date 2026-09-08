#include "../../include/evaluation.hpp"
#include "../../league/public_router/source/agent.hpp"
#include "policy.hpp"
#include <filesystem>

using namespace compositions;

template<int Mode> struct Observed {
    cold_day_tasks::Agent<Mode> base;
    int used=0,first_mismatch=-1;bool enabled=true;
    std::array<int,24> actual_units{},planned_units{};
    void reset(const kag::agent::AgentInit& init){base.reset(init);used=0;first_mismatch=-1;enabled=true;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){
        base.act(o,b,a);
        if(o.day!=0)return;
        const auto& p=cold_day_tasks::days[Mode][o.hour];actual_units[o.hour]=o.self().n_units;planned_units[o.hour]=p.n_units;
        if(enabled && p.n_units!=o.self().n_units){enabled=false;first_mismatch=o.hour;}
        if(enabled){uint64_t x=0,y=0;hash_action(x,a);hash_action(y,p);if(x!=y)std::abort();++used;}
    }
};

template<int Mode,class Rival> void run(const std::filesystem::path& directory,const std::string& rival,std::ostream& coverage){
    Options o;o.validate=true;o.profile=true;o.a=Mode==0?"cold_day_tasks_full":Mode==1?"cold_day_tasks_sheep_service":"cold_day_tasks_establish";o.b=rival;
    o.output=(directory/(o.a+"_vs_"+rival+".json")).string();
    Observed<Mode> own;Rival opponent;std::vector<Outcome> games;
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat){
        games.push_back(run_game(own,opponent,seed,seat,o));
        coverage<<Mode<<','<<rival<<','<<seed<<','<<seat<<','<<own.used<<','<<own.first_mismatch;
        for(int h=0;h<24;++h)coverage<<','<<own.actual_units[h];coverage<<'\n';
    }
    write_results(o,games,0);
}

int main(int argc,char** argv){
    if(argc!=2)return 2;const std::filesystem::path directory=argv[1];if(std::filesystem::exists(directory))return 2;
    std::filesystem::create_directories(directory);
    std::ofstream coverage(directory/"coverage.csv");coverage<<"mode,opponent,seed,seat,scheduled_hours,first_mismatch";
    for(int h=0;h<24;++h)coverage<<",units_h"<<h;coverage<<'\n';
    run<0,public_router::Agent>(directory,"public_router",coverage);run<1,public_router::Agent>(directory,"public_router",coverage);run<2,public_router::Agent>(directory,"public_router",coverage);
    run<0,Pass>(directory,"pass",coverage);run<1,Pass>(directory,"pass",coverage);run<2,Pass>(directory,"pass",coverage);
}
