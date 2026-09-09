#include "../../include/evaluation.hpp"
#include "proposals/titan_lots_h0_m1/source/agent.hpp"
#include "proposals/titan_lots_h4_m1/source/agent.hpp"
#include "proposals/titan_lots_h8_m1/source/agent.hpp"
#include "proposals/titan_lots_h8_m2/source/agent.hpp"
#include <filesystem>
using namespace compositions;
namespace fs=std::filesystem;
using Observation=kag::agent::AgentObservation;
bool units_equal(const kag::Action& a,const kag::Action& b) {
    if(a.n_units!=b.n_units)return false;
    for(int u=0;u<a.n_units;++u)if(a.units[u].op!=b.units[u].op || a.units[u].arg!=b.units[u].arg || a.units[u].n!=b.units[u].n)return false;
    return true;
}
struct Entry {Observation observation;kag::Action action;};
template<class Base> class Observed:public Base {
    empty_sale_slots_m2::Agent shadow_;
public:
    int witness=-1;bool capture=false;Entry entry;
    const Entry* baseline=nullptr;
    bool actual_equal=false,stock_equal=false,cash_equal=false;
    int producer_units_equal=0;
    void reset(const kag::agent::AgentInit& i){Base::reset(i);shadow_.reset(i);producer_units_equal=0;}
    void act(const Observation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        auto before=shadow_;kag::Action producer;shadow_.act(o,b,producer);Base::act(o,b,a);
        if(!units_equal(producer,a))std::abort();++producer_units_equal;
        if(o.step!=witness)return;
        if(capture){entry={o,a};return;}
        actual_equal=units_equal(a,baseline->action);
        auto changed=o;std::copy_n(baseline->observation.own.shed,kag::N_ITEMS,changed.own.shed);
        changed.own.shed_total=baseline->observation.own.shed_total;
        auto corrected=before;kag::Action after;corrected.act(changed,b,after);stock_equal=units_equal(after,baseline->action);
        changed=o;changed.farms[o.player].money=baseline->observation.self().money;
        corrected=before;corrected.act(changed,b,after);cash_equal=units_equal(after,baseline->action);
    }
};
template<class Own> void test(const fs::path& out,std::ofstream& summary,const std::string& name,int seed,int step) {
    Observed<titan_lots_h0_m1::Agent> control;empty_sale_slots_m2::Agent rival;
    control.witness=step;control.capture=true;
    Options options;options.a="titan_lots_h0_m1";options.b="empty_sale_slots_m2";options.validate=true;options.profile=true;options.expansions=100000;
    options.output=(out/(name+"_"+std::to_string(seed)+"_control.json")).string();
    std::vector<Outcome> baseline;baseline.push_back(run_game(control,rival,seed,0,options));write_results(options,baseline,0);
    Observed<Own> own;own.witness=step;own.baseline=&control.entry;
    options.a=name;options.output=(out/(name+"_"+std::to_string(seed)+".json")).string();
    std::vector<Outcome> actual;actual.push_back(run_game(own,rival,seed,0,options));write_results(options,actual,0);
    summary<<"{\"agent\":\""<<name<<"\",\"seed\":"<<seed<<",\"step\":"<<step
        <<",\"actual_units_equal\":"<<own.actual_equal<<",\"stock_only_restores_units\":"<<own.stock_equal
        <<",\"cash_only_restores_units\":"<<own.cash_equal<<",\"producer_calls_equal\":"<<own.producer_units_equal<<"}\n";
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;fs::path out=argv[1];if(fs::exists(out))return 2;fs::create_directories(out);
    std::ofstream summary(out/"witnesses.jsonl");
    test<titan_lots_h4_m1::Agent>(out,summary,"titan_lots_h4_m1",1006,552);
    test<titan_lots_h8_m1::Agent>(out,summary,"titan_lots_h8_m1",1006,480);
    test<titan_lots_h8_m2::Agent>(out,summary,"titan_lots_h8_m2",1006,264);
    test<titan_lots_h4_m1::Agent>(out,summary,"titan_lots_h4_m1",1011,672);
    test<titan_lots_h8_m1::Agent>(out,summary,"titan_lots_h8_m1",1011,624);
    test<titan_lots_h8_m2::Agent>(out,summary,"titan_lots_h8_m2",1011,264);
}
