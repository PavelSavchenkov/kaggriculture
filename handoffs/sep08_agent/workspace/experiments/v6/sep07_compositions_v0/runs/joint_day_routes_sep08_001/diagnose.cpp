#include "../../include/evaluation.hpp"
#include "proposals/joint_routes_p355_m0/source/agent.hpp"
#include "proposals/joint_routes_p355_m1/source/agent.hpp"
#include "proposals/joint_routes_p362_m0/source/agent.hpp"
#include "proposals/joint_routes_p362_m1/source/agent.hpp"
#include "../../league/public_router/source/agent.hpp"
#include <filesystem>

using namespace compositions;
namespace fs=std::filesystem;

template<class Base> class Observed:public Base {
public:
    std::ostream* trace=nullptr;
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {
        Base::act(o,b,a);
        if(o.day<14 || o.day>16)return;
        typename Base::MarketContext c;kag::Action unused;
        this->plan_units(o,unused,c);
        int counts[18]{};
        for(int cell=0;cell<100;++cell)for(int j=0;j<c.task_count[cell];++j)++counts[c.tasks[cell][j].action.op];
        auto& out=*trace;
        out<<"{\"step\":"<<o.step<<",\"cash\":"<<o.self().money<<",\"shed\":[";
        for(int item=0;item<kag::N_ITEMS;++item){if(item)out<<',';out<<+o.own.shed[item];}
        out<<"],\"pending\":[";for(int i=0;i<18;++i){if(i)out<<',';out<<counts[i];}
        out<<"],\"workers\":[";
        for(int u=0;u<o.self().n_units;++u) {
            if(u)out<<',';const auto& v=a.units[u];
            out<<'['<<+o.self().pos_x[u]<<','<<+o.self().pos_y[u]<<','<<+o.own.inv[u][kag::WHEAT]<<','
                <<+o.own.inv[u][kag::FERTILIZER]<<','<<+v.op<<','<<+v.arg<<','<<v.n<<']';
        }
        out<<"],\"tasks\":[";int n=0;
        for(int cell=0;cell<100;++cell)for(int j=0;j<c.task_count[cell];++j) {
            if(n++)out<<',';const auto& task=c.tasks[cell][j];
            out<<'['<<cell<<','<<+task.action.op<<','<<task.input<<','<<task.deadline<<']';
        }
        out<<"],\"tiles\":[";
        for(int cell=0;cell<100;++cell) {
            if(cell)out<<',';const auto& t=o.self().tiles[cell/10][cell%10];
            out<<'['<<+t.kind<<','<<+t.what<<','<<t.planted_day<<','<<+t.yield_units<<','<<+t.consecutive_dry
                <<','<<t.watered_today<<','<<t.fed_today<<','<<t.cared_today<<','<<t.fertilized_until_day<<']';
        }
        out<<"]}\n";
    }
};

template<class Base> void batch(const fs::path& dir,const std::string& name) {
    Options options;options.a=name;options.b="public_router";options.validate=true;options.profile=true;
    options.output=(dir/(name+"_vs_public_router.json")).string();
    Observed<Base> own;public_router::Agent rival;std::vector<Outcome> games;
    auto start=std::chrono::steady_clock::now();
    for(int seed=1000;seed<1002;++seed)for(int seat=0;seat<2;++seat) {
        std::ofstream trace(dir/(name+"_"+std::to_string(seed)+"_s"+std::to_string(seat)+".jsonl"));
        own.trace=&trace;games.push_back(run_game(own,rival,seed,seat,options));
    }
    write_results(options,games,std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count());
}

int main(int argc,char** argv) {
    if(argc!=2)return 2;fs::path dir=argv[1];if(fs::exists(dir))return 2;fs::create_directories(dir);
    batch<joint_routes_p355_m0::Agent>(dir,"joint_routes_p355_m0");
    batch<joint_routes_p355_m1::Agent>(dir,"joint_routes_p355_m1");
    batch<joint_routes_p362_m0::Agent>(dir,"joint_routes_p362_m0");
    batch<joint_routes_p362_m1::Agent>(dir,"joint_routes_p362_m1");
}
