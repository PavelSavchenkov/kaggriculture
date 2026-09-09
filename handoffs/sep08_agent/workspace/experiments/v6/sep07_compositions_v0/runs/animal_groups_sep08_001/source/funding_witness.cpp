#include "season.hpp"

std::array<Action,24> read_actions(const fs::path& path){
    std::ifstream input(path);if(!input)std::abort();std::array<Action,24> result;
    for(auto& a:result){
        input>>a.n_units>>a.n_orders;
        for(int u=0;u<a.n_units;++u){int op,arg;input>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
        for(int j=0;j<a.n_orders;++j){int op,item;input>>op>>item>>a.orders[j].n;a.orders[j].op=op;a.orders[j].item=item;}
        a.finalize();
    }
    if(!input)std::abort();return result;
}

int main(int argc,char** argv){
    if(argc!=5 || fs::exists(argv[4]))return 2;
    const uint64_t seed=std::stoull(argv[1]);const int day=std::stoi(argv[2]);
    const auto actions=read_actions(argv[3]);
    Config config;config.seed=seed;Sim sim(config);Source own;public_router::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    uint64_t random=seed^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops;
    for(auto& s:shops)s=random_word(random)%N_SHOPS;
    std::ofstream out(argv[4]);out<<"hour,slot,item,requested,accepted,cash_before_order,cash_after_order,shed_before,shed_after\n";
    while(!sim.st.done && sim.st.day<=day){
        std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        Action pair[2];own.act(agent::runtime::make_observation(sim,0),decision_budget(),pair[0]);
        rival.act(agent::runtime::make_observation(sim,1),decision_budget(),pair[1]);
        if(sim.st.day==day){
            pair[0]=actions[sim.st.hour];
            auto before=prefix_phase(sim,pair,0);
            for(int j=0;j<pair[0].n_orders;++j){
                auto after=prefix_phase(sim,pair,j+1);const auto& order=pair[0].orders[j];
                if(order.op==M_BUY_ANIMAL)out<<sim.st.hour<<','<<j<<','<<+order.item<<','<<order.n<<','
                    <<after.st.farms[0].shed[order.item]-before.st.farms[0].shed[order.item]<<','
                    <<before.st.farms[0].money<<','<<after.st.farms[0].money<<','
                    <<before.st.farms[0].shed[order.item]<<','<<after.st.farms[0].shed[order.item]<<'\n';
                before=std::move(after);
            }
        }
        sim.step(pair[0],pair[1]);
    }
}
