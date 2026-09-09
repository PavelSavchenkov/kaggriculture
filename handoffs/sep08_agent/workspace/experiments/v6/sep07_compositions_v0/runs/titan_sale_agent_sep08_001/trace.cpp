#include "../../include/evaluation.hpp"
#include "proposals/titan_lots_h0_m1/source/agent.hpp"
#include "proposals/titan_lots_h4_m1/source/agent.hpp"
#include "proposals/titan_lots_h8_m1/source/agent.hpp"
#include "proposals/titan_lots_h8_m2/source/agent.hpp"
#include <filesystem>
using namespace compositions;
namespace fs=std::filesystem;
template<class Base> class Traced:public Base {
public:
    std::ofstream trace;double max_ms=0,total_ms=0;
    void reset(const kag::agent::AgentInit& i){Base::reset(i);max_ms=total_ms=0;}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        const auto start=std::chrono::steady_clock::now();Base::act(o,b,a);
        const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        max_ms=std::max(max_ms,ms);total_ms+=ms;
        trace<<"{\"step\":"<<o.step<<",\"cash\":"<<o.self().money<<",\"chosen\":"<<this->chosen()<<",\"changed\":"<<this->changed()
            <<",\"rejected\":"<<this->rejected()<<",\"gain\":"<<this->predicted_gain()<<",\"shed\":[";
        for(int i=0;i<kag::N_ITEMS;++i){if(i)trace<<',';trace<<o.own.shed[i];}
        trace<<"],\"units\":[";
        for(int u=0;u<a.n_units;++u){if(u)trace<<',';trace<<'['<<int(a.units[u].op)<<','<<int(a.units[u].arg)<<','<<a.units[u].n<<']';}
        trace<<"],\"orders\":[";
        for(int j=0;j<a.n_orders;++j){if(j)trace<<',';trace<<'['<<int(a.orders[j].op)<<','<<int(a.orders[j].item)<<','<<a.orders[j].n<<']';}
        trace<<"]}\n";
    }
};
template<class Base> void run(const fs::path& out,const std::string& name,int seed) {
    Traced<Base> own;empty_sale_slots_m2::Agent rival;
    const auto prefix=name+"_"+std::to_string(seed);own.trace.open(out/(prefix+".jsonl"));
    Options options;options.a=name;options.b="empty_sale_slots_m2";options.validate=true;options.profile=true;
    options.expansions=100000;options.output=(out/(prefix+".json")).string();
    std::vector<Outcome> results;results.push_back(run_game(own,rival,seed,0,options));write_results(options,results,0);
    std::ofstream stats(out/(prefix+".stats.json"));stats<<"{\"max_ms\":"<<own.max_ms<<",\"total_ms\":"<<own.total_ms
        <<",\"chosen\":"<<own.chosen()<<",\"changed\":"<<own.changed()<<",\"rejected\":"<<own.rejected()
        <<",\"evaluated\":"<<own.evaluated()<<",\"forecasts\":"<<own.forecasts()<<",\"budget_stops\":"<<own.budget_stops()<<"}\n";
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;const fs::path out=argv[1];if(fs::exists(out))return 2;fs::create_directories(out);
    for(int seed:{1006,1011}) {
        run<titan_lots_h0_m1::Agent>(out,"titan_lots_h0_m1",seed);
        run<titan_lots_h4_m1::Agent>(out,"titan_lots_h4_m1",seed);
        run<titan_lots_h8_m1::Agent>(out,"titan_lots_h8_m1",seed);
        run<titan_lots_h8_m2::Agent>(out,"titan_lots_h8_m2",seed);
    }
}
