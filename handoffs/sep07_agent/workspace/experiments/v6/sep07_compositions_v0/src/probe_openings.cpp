#include "evaluation.hpp"
#include "opening_features.hpp"
#include "registry.hpp"

int main(int argc,char** argv) {
    if(argc<3)return 2;
    std::ofstream output(argv[1]);if(!output)return 2;
    output<<"opponent,seat,money,workers,land,farmer_x,farmer_y,pastures,coops,wheat,carrot,tomato,strawberry,melon,goose,cow,sheep\n";
    for(int i=2;i<argc;++i)for(int seat=0;seat<2;++seat) {
        kag::Config config;config.seed=1000;kag::Sim sim(config);
        auto a=make_agent("public_router"),b=make_agent(argv[i]),teacher=make_agent("feeltheagi_55");
        auto init=kag::agent::runtime::make_agent_init(sim,seat);
        a.reset(init);teacher.reset(init);b.reset(kag::agent::runtime::make_agent_init(sim,seat^1));
        auto obs=kag::agent::runtime::make_observation(sim,seat);
        auto rival=kag::agent::runtime::make_observation(sim,seat^1);
        kag::Action actions[2],alternative;a.act(obs,{},actions[seat]);teacher.act(obs,{},alternative);b.act(rival,{},actions[seat^1]);
        uint64_t h1=0,h2=0;compositions::hash_action(h1,actions[seat]);compositions::hash_action(h2,alternative);
        if(h1!=h2) {std::fprintf(stderr,"common opening differs\n");return 1;}
        sim.step(actions[0],actions[1]);
        auto f=compositions::opening_features(kag::agent::runtime::make_observation(sim,seat));
        output<<argv[i]<<','<<seat<<','<<f.money<<','<<f.workers<<','<<f.land<<','<<f.farmer_x<<','<<f.farmer_y<<','<<f.pastures<<','<<f.coops;
        for(int v:f.crops)output<<','<<v;
        for(int v:f.animals)output<<','<<v;
        output<<'\n';
    }
}
