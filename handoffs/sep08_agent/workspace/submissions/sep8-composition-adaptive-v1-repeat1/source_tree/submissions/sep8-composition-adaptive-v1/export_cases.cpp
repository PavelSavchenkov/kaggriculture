#include "experiments/v6/sep07_compositions_v0/include/evaluation.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/animal_repair_sep08_001/proposals/animal_repair_q24_premium_m2/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/king_rc4/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/john_131/source/agent.hpp"
#include <iomanip>
#include <iostream>
using namespace compositions;
using Target=animal_repair_q24_premium_m2::Agent;

void pack(std::ostream& out,const kag::agent::AgentObservation& o,const kag::Action& a){
    out<<std::setprecision(17)<<int(o.step==0)<<' '<<+o.player<<' '<<o.step<<' '<<o.day<<' '<<o.hour;
    for(const auto& f:o.farms){
        out<<' '<<f.money<<' '<<f.n_units<<' '<<f.n_quadrants<<' '<<f.hires_today;
        for(int u=0;u<f.n_units;++u)out<<' '<<+f.pos_x[u]<<' '<<+f.pos_y[u];
        for(const auto& row:f.tiles)for(const auto& t:row)
            out<<' '<<+t.kind<<' '<<+t.what<<' '<<+t.has_animal<<' '<<+t.watered_today<<' '<<+t.fed_today
                <<' '<<+t.cared_today<<' '<<+t.fertilizer_available<<' '<<+t.consecutive_dry<<' '<<+t.yield_units
                <<' '<<+t.pending_care_bonus<<' '<<t.planted_day<<' '<<t.max_lifespan_step<<' '<<t.fertilized_until_day;
    }
    for(auto n:o.own.shed)out<<' '<<+n;for(auto n:o.own.seeds)out<<' '<<+n;
    for(int u=0;u<o.self().n_units;++u){out<<' '<<+o.own.inv_nkeys[u];for(int k=0;k<o.own.inv_nkeys[u];++k){int item=o.own.inv_keys[u][k];out<<' '<<item<<' '<<o.own.inv[u][item];}}
    for(auto n:o.market.prices)out<<' '<<n;for(auto n:o.market.inventory)out<<' '<<n;
    out<<' '<<o.n_shops;for(int s=0;s<o.n_shops;++s)out<<' '<<+o.shops[s];
    out<<' '<<a.n_units<<' '<<a.n_orders;
    for(int u=0;u<a.n_units;++u)out<<' '<<+a.units[u].op<<' '<<+a.units[u].arg<<' '<<a.units[u].n;
    for(int s=0;s<a.n_orders;++s)out<<' '<<+a.orders[s].op<<' '<<+a.orders[s].item<<' '<<a.orders[s].n;
    out<<'\n';
}
struct Recorder {
    Target value;
    std::ofstream& out;
    static kag::agent::AgentInfo info(){return Target::info();}
    void reset(const kag::agent::AgentInit& init){value.reset(init);}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a){value.act(o,b,a);pack(out,o,a);}
};
int main(int argc,char**argv){
    if(argc!=5)return 2;Options options;options.validate=true;options.native_shops=false;
    const uint64_t seed=std::stoull(argv[1]);const int seat=std::stoi(argv[2]);
    std::ofstream out(argv[4]);if(!out)return 2;Recorder own{Target{},out};Outcome result;
    const std::string rival_name=argv[3];
    if(rival_name=="king"){king_rc4::Agent rival;result=run_game(own,rival,seed,seat,options);}
    else if(rival_name=="john"){john_131::Agent rival;result=run_game(own,rival,seed,seat,options);}
    else return 2;
    const auto& d=own.value.diagnostics();
    std::cout<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"cash\":"<<result.cash[seat]<<",\"rival_cash\":"<<result.cash[seat^1]
        <<",\"family\":"<<d.family<<",\"choice\":"<<d.choice<<",\"repaired_days\":"<<d.repaired_days<<",\"missed_days\":"<<d.missed_days<<"}\n";
}
