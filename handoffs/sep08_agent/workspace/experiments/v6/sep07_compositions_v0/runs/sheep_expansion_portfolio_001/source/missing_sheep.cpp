#include "agent.hpp"
#include "experiments/v6/sep07_compositions_v0/league/teammate_shoprouter/source/agent.hpp"
#include "experiments/v6/sep07_compositions_v0/include/evaluation.hpp"
#include <fstream>

int main(int argc,char**argv){
    if(argc!=2)return 2;std::ofstream out(argv[1]);kag::Config cfg;cfg.seed=1004;kag::Sim sim(cfg);
    compositions::sheep_portfolio::Agent a(1);compositions::teammate_shoprouter::Agent b;
    a.reset(kag::agent::runtime::make_agent_init(sim,0));b.reset(kag::agent::runtime::make_agent_init(sim,1));
    uint64_t rng=cfg.seed^0xa37108e62d045fb9ULL;int shops[8];for(auto&s:shops)s=compositions::random_word(rng)%8;
    out<<"[";bool comma=false;
    while(!sim.st.done){
        std::copy_n(shops,sim.st.n_shops,sim.st.shops);kag::Action actions[2];const auto o=kag::agent::runtime::make_observation(sim,0);a.act(o,{},actions[0]);b.act(kag::agent::runtime::make_observation(sim,1),{},actions[1]);
        if(sim.st.step>=340&&sim.st.step<=400){
            if(comma)out<<",";comma=true;out<<"{\"step\":"<<sim.st.step<<",\"money\":"<<o.self().money<<",\"shed_sheep\":"<<o.own.shed[kag::SHEEP]<<",\"cells\":[";
            const auto accepted=sim.sanitize_solo_action(0,actions[0]);
            for(int cell=0;cell<2;++cell){if(cell)out<<",";int x=cell?2:1,y=cell?2:3;const auto&t=o.self().tiles[y][x];out<<"{\"x\":"<<x<<",\"y\":"<<y<<",\"kind\":"<<int(t.kind)<<",\"has_animal\":"<<int(t.has_animal)<<",\"units\":[";bool uc=false;
                for(int u=0;u<o.self().n_units;++u)if(o.self().pos_x[u]==x&&o.self().pos_y[u]==y){if(uc)out<<",";uc=true;out<<"["<<u<<","<<int(actions[0].units[u].op)<<","<<int(accepted.units[u].op)<<","<<o.own.inv[u][kag::SHEEP]<<"]";}out<<"]}";}
            out<<"]}";
        }sim.step(actions[0],actions[1]);
    }out<<"]\n";
}
