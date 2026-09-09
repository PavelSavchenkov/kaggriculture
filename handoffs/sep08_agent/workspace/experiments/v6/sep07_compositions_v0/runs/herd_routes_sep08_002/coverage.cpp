#include "../../include/evaluation.hpp"
#include "proposals/herd_partial_mixed_m2/source/agent.hpp"
#include "proposals/herd_partial_goose_m2/source/agent.hpp"
#include "proposals/herd_partial_p355_m2/source/agent.hpp"
#include "proposals/herd_partial_p362_m2/source/agent.hpp"
#include "../../league/public_router/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;

template<class Base> class Observed:public Base {
    std::array<int,kag::MAX_UNITS> previous_op_{},previous_wheat_{};
    int last_step_=-2;
public:
    std::array<int,30> hours{};
    std::vector<std::array<int,8>> reversals;
    void reset(const kag::agent::AgentInit& init) {
        Base::reset(init);hours.fill(0);reversals.clear();previous_op_.fill(0);previous_wheat_.fill(0);last_step_=-2;
    }
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        int before=this->routed_turns();Base::act(o,b,a);
        if(this->routed_turns()==before){last_step_=-2;return;}
        ++hours[o.day];
        for(int u=0;u<o.self().n_units;++u) {
            int op=a.units[u].op,prev=previous_op_[u],wheat=o.own.inv[u][kag::WHEAT];
            bool reversed=(op==kag::OP_NORTH && prev==kag::OP_SOUTH)||(op==kag::OP_SOUTH && prev==kag::OP_NORTH)
                ||(op==kag::OP_EAST && prev==kag::OP_WEST)||(op==kag::OP_WEST && prev==kag::OP_EAST);
            if(last_step_==o.step-1 && reversed && wheat>0 && wheat==previous_wheat_[u])
                reversals.push_back({o.step,u,o.self().pos_x[u],o.self().pos_y[u],wheat,o.own.shed[kag::WHEAT],op,prev});
            previous_op_[u]=op;previous_wheat_[u]=wheat;
        }
        last_step_=o.step;
    }
};

template<class Base> void batch(const fs::path& out,const std::string& name) {
    Options options;options.a=name;options.b="public_router";options.validate=true;options.profile=true;
    options.output=(out/(name+"_vs_public_router.json")).string();
    Observed<Base> own;public_router::Agent rival;std::vector<Outcome> games;
    std::ofstream coverage(out/(name+".coverage.jsonl"));
    auto start=std::chrono::steady_clock::now();
    for(int seed=1000;seed<1008;++seed)for(int seat=0;seat<2;++seat) {
        games.push_back(run_game(own,rival,seed,seat,options));
        coverage<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"days\":"<<own.routed_days()<<",\"hours\":[";
        for(int d=0;d<30;++d){if(d)coverage<<',';coverage<<own.hours[d];}
        coverage<<"],\"reversals\":[";
        for(size_t i=0;i<own.reversals.size();++i) {
            if(i)coverage<<',';coverage<<'[';
            for(int k=0;k<8;++k){if(k)coverage<<',';coverage<<own.reversals[i][k];}coverage<<']';
        }
        coverage<<"]}\n";
    }
    write_results(options,games,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;fs::path out=argv[1];if(fs::exists(out))return 2;fs::create_directories(out);
    batch<herd_partial_mixed_m2::Agent>(out,"herd_partial_mixed_m2");
    batch<herd_partial_goose_m2::Agent>(out,"herd_partial_goose_m2");
    batch<herd_partial_p355_m2::Agent>(out,"herd_partial_p355_m2");
    batch<herd_partial_p362_m2::Agent>(out,"herd_partial_p362_m2");
}
