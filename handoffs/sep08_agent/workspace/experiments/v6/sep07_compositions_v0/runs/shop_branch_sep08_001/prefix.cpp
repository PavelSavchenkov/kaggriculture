#include "../../include/evaluation.hpp"
#include "registry.hpp"
#include <fstream>

using namespace kag;
using namespace compositions;
bool equal(const Action&a,const Action&b) {
    if(a.n_units!=b.n_units||a.n_orders!=b.n_orders)return false;
    for(int u=0;u<a.n_units;++u)if(a.units[u].op!=b.units[u].op||a.units[u].arg!=b.units[u].arg||a.units[u].n!=b.units[u].n)return false;
    for(int j=0;j<a.n_orders;++j)if(a.orders[j].op!=b.orders[j].op||a.orders[j].item!=b.orders[j].item||a.orders[j].n!=b.orders[j].n)return false;
    return true;
}
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    const char* opponents[]={"empty_sale_slots_m2","teammate_shoprouter","public_router","public_router_v52","joint_routes_p362_m0","service_bank_p362_m2"};
    std::ofstream out(argv[1]);out<<"{\"prefixes\":[";bool first=true;
    int total=0,passed=0;
    for(int native=0;native<2;++native)for(int i=0;i<(native?8:128);++i)for(int seat=0;seat<2;++seat)for(const char* opponent:opponents) {
        Config config;config.seed=2400000+i;Sim sim(config);
        auto base=make_agent("service_bank_p362_m2"),alternative=make_agent("cold_renewal_p98"),rival=make_agent(opponent);
        auto init=agent::runtime::make_agent_init(sim,seat);base.reset(init);alternative.reset(init);rival.reset(agent::runtime::make_agent_init(sim,seat^1));
        agent::DecisionBudget budget;budget.max_expansions=100000;
        uint64_t rng=config.seed^0xa37108e62d045fb9ULL;std::array<uint8_t,8> shops{};
        for(auto& s:shops)s=random_word(rng)%N_SHOPS;
        int mismatch=-1;
        for(int step=0;step<144;++step) {
            if(!native)std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            auto own=agent::runtime::make_observation(sim,seat),other=agent::runtime::make_observation(sim,seat^1);
            Action acts[2],candidate;
            base.act(own,budget,acts[seat]);alternative.act(own,budget,candidate);rival.act(other,budget,acts[seat^1]);
            validate_action(acts[seat],own);validate_action(candidate,own);validate_action(acts[seat^1],other);
            if(!equal(acts[seat],candidate)){mismatch=step;break;}
            sim.step(acts[0],acts[1]);
        }
        if(!native)std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
        const auto own=agent::runtime::make_observation(sim,seat);
        int animals[3]{};for(const auto& row:own.opponent().tiles)for(const auto& t:row)if(t.has_animal)++animals[t.what-GOOSE];
        if(!first)out<<',';first=false;
        out<<"{\"seed\":"<<config.seed<<",\"seat\":"<<seat<<",\"native\":"<<native<<",\"opponent\":\""<<opponent
           <<"\",\"mismatch\":"<<mismatch<<",\"step\":"<<own.step<<",\"shops\":[";
        for(int s=0;s<own.n_shops;++s){if(s)out<<',';out<<int(own.shops[s]);}
        out<<"],\"own_cash\":"<<own.self().money<<",\"rival_cash\":"<<own.opponent().money<<",\"prices\":[";
        for(int item=0;item<N_PRODUCTS;++item){if(item)out<<',';out<<own.market.prices[item];}
        out<<"],\"rival_animals\":["<<animals[0]<<','<<animals[1]<<','<<animals[2]<<"]}";
        ++total;passed+=mismatch<0;
    }
    out<<"],\"total\":"<<total<<",\"equal_through_step143\":"<<passed<<"}\n";
    return total==passed?0:3;
}
