#include "../../include/evaluation.hpp"
#include "../day_programs_sep08_001/proposals/day_program_p362_m0/source/agent.hpp"
#include "../day_programs_sep08_001/proposals/day_program_p362_m3/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;

template<class Base> class Traced:public Base {
public:
    std::ofstream trace;
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        Base::act(o,b,a);
        if(o.day<12 || o.day>19)return;
        trace<<"{\"step\":"<<o.step<<",\"day\":"<<o.day<<",\"hour\":"<<o.hour<<",\"active\":"<<this->active_hours()
            <<",\"cash\":"<<o.self().money<<",\"shed_wheat\":"<<o.own.shed[kag::WHEAT]<<",\"tiles\":[";
        bool comma=false;
        for(int cell=0;cell<100;++cell) {
            const auto& t=o.self().tiles[cell/10][cell%10];
            if(!t.has_animal && t.kind!=kag::T_PLANT)continue;
            if(comma)trace<<',';comma=true;
            trace<<'['<<cell<<','<<int(t.kind)<<','<<int(t.what)<<','<<t.has_animal<<','<<o.day-t.planted_day
                <<','<<int(t.yield_units)<<','<<int(t.consecutive_dry)<<','<<int(t.pending_care_bonus)
                <<','<<t.fed_today<<','<<t.cared_today<<','<<t.watered_today<<','<<t.fertilized_until_day<<']';
        }
        trace<<"],\"units\":[";
        for(int u=0;u<a.n_units;++u) {
            if(u)trace<<',';
            trace<<'['<<u<<','<<int(o.self().pos_x[u])<<','<<int(o.self().pos_y[u])<<','<<int(a.units[u].op)
                <<','<<int(a.units[u].arg)<<','<<a.units[u].n<<','<<o.own.inv[u][kag::WHEAT]<<']';
        }
        trace<<"]}\n";
    }
};

template<class Base> void run(const fs::path& out,const std::string& name,int seed,int seat) {
    Traced<Base> own;joint_routes_p362_m0::Agent rival;
    const auto prefix=name+"_"+std::to_string(seed)+"_"+std::to_string(seat);
    own.trace.open(out/(prefix+".jsonl"));
    Options options;options.a=name;options.b="joint_routes_p362_m0";options.validate=true;options.profile=true;
    options.expansions=100000;options.output=(out/(prefix+".json")).string();
    std::vector<Outcome> results;results.push_back(run_game(own,rival,seed,seat,options));
    write_results(options,results,0);
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;const fs::path out=argv[1];if(fs::exists(out))return 2;fs::create_directories(out);
    for(int seed:{1001,1002,1006})for(int seat=0;seat<2;++seat) {
        run<day_program_p362_m0::Agent>(out,"day_program_p362_m0",seed,seat);
        run<day_program_p362_m3::Agent>(out,"day_program_p362_m3",seed,seat);
    }
}
