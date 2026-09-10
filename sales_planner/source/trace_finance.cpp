#include "evaluation.hpp"
#include "registry.hpp"

int main(int argc,char** argv) {
    const auto o=compositions::options(argc,argv);
    auto agent=make_agent(o.a), rival=make_agent(o.b);
    kag::Config config;config.seed=o.seed_start;
    kag::Sim sim(config);
    agent.reset(kag::agent::runtime::make_agent_init(sim,0));
    rival.reset(kag::agent::runtime::make_agent_init(sim,1));
    kag::agent::DecisionBudget budget;budget.max_expansions=o.expansions;
    uint64_t shop_rng=o.seed_start^0xa37108e62d045fb9ULL;
    std::array<uint8_t,8> shops;
    for(auto& s:shops)s=compositions::random_word(shop_rng)%kag::N_SHOPS;
    std::ofstream file(o.output);if(!file)std::abort();
    while(!sim.st.done) {
        if(!o.native_shops)std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        const auto obs=kag::agent::runtime::make_observation(sim,0);
        kag::Action actions[2];agent.act(obs,budget,actions[0]);
        rival.act(kag::agent::runtime::make_observation(sim,1),budget,actions[1]);
        compositions::validate_action(actions[0],obs);
        const auto d=sim.diagnose_joint_actions(actions[0],actions[1]);
        file<<"{\"turn\":"<<sim.st.step<<",\"cash\":"<<sim.st.farms[0].money<<",\"shed\":[";
        for(int i=0;i<kag::N_ITEMS;++i){if(i)file<<',';file<<obs.own.shed[i];}
        file<<"],\"units\":[";
        for(int u=0;u<actions[0].n_units;++u) {
            if(u)file<<',';const auto a=actions[0].units[u];
            file<<'['<<+a.op<<','<<+a.arg<<','<<a.n<<']';
        }
        file<<"],\"orders\":[";
        for(int k=0;k<actions[0].n_orders;++k) {
            if(k)file<<',';const auto a=actions[0].orders[k];
            file<<'['<<+a.op<<','<<+a.item<<','<<a.n<<']';
        }
        file<<"],\"unit_faults\":"<<d.players[0].requested_unit_actions-d.players[0].successful_unit_actions;
        sim.step(actions[0],actions[1]);
        file<<",\"after_cash\":"<<sim.st.farms[0].money<<",\"after_workers\":"<<sim.st.farms[0].n_units<<"}\n";
    }
}
