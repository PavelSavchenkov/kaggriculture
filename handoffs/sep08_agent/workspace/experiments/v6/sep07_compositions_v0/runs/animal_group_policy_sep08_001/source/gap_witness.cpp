// Offline causal opening intervention; production policies remain unchanged.
#include "season.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/yusuke_port_sep08_001/proposals/yusuke_sep08_m2/source/agent.hpp"
void run(int mode){
    Config config;config.seed=1000;Sim sim(config);Source own;compositions::yusuke_sep08_m2::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    uint64_t rng=1000^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops;for(auto& s:shops)s=random_word(rng)%N_SHOPS;
    DetailedProfile profile;uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};
    while(!sim.st.done){
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);Action acts[2];
        own.act(agent::runtime::make_observation(sim,0),decision_budget(),acts[0]);
        rival.act(agent::runtime::make_observation(sim,1),decision_budget(),acts[1]);
        const int step=sim.st.step;
        if((mode==1 && step<2) || (mode==2 && step==0)){
            const auto& tape=compositions::yusuke_port::planned(0,step);
            acts[0].n_orders=tape.n_orders;std::copy_n(tape.orders,tape.n_orders,acts[0].orders);acts[0].finalize();
        }
        if(step<3){
            std::cout<<"mode="<<mode<<" step="<<step;
            for(int p=0;p<2;++p){const auto& f=sim.st.farms[p];std::cout<<" player="<<p<<" cash="<<f.money<<" wheat="<<f.shed[WHEAT]<<" orders=";
                for(int s=0;s<acts[p].n_orders;++s){const auto& o=acts[p].orders[s];std::cout<<+o.op<<':'<<+o.item<<':'<<o.n<<';';}}
            std::cout<<'\n';
        }
        for(int p=0;p<2;++p){validate_action(acts[p],agent::runtime::make_observation(sim,p));hash_action(hashes[p],acts[p]);}
        const auto before=sim;sim.step(acts[0],acts[1]);profile.observe(before,sim,acts);
        for(int c=0;c<100;++c){const auto& a=before.st.farms[0].tiles[c/10][c%10];const auto& b=sim.st.farms[0].tiles[c/10][c%10];
            if(a.has_animal && !b.has_animal)std::cout<<"mode="<<mode<<" escaped step="<<step<<" cell="<<c<<" item="<<+a.what<<" dry="<<a.consecutive_dry<<'\n';}
        if(step<72 && step%24==23){const auto& f=sim.st.farms[0];
            int cows=0;for(int c=0;c<100;++c)cows+=f.tiles[c/10][c%10].has_animal && f.tiles[c/10][c%10].what==COW;
            std::cout<<"mode="<<mode<<" endstep="<<step<<" cash="<<f.money<<" wheat="<<f.shed[WHEAT]<<" cowstock="<<f.shed[COW]<<" livecows="<<cows<<'\n';}
    }
    std::cout<<"RESULT mode="<<mode<<" cash="<<sim.st.farms[0].money<<" rival="<<sim.st.farms[1].money
        <<" hires="<<profile.farms[0].hires<<" hire_cost="<<profile.farms[0].hire_cost
        <<" hashes="<<hashes[0]<<':'<<hashes[1]<<" produced=";
    for(int i=0;i<N_ITEMS;++i)std::cout<<sim.st.farms[0].produced[i]<<',';std::cout<<'\n';
}
int main(){for(int mode=0;mode<3;++mode)run(mode);}
