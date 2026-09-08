#include "../../include/evaluation.hpp"
#include "registry.hpp"
#include <fstream>

using namespace kag;
using namespace compositions;
bool same(const Action& a,const Action& b) {
    if(a.n_units!=b.n_units || a.n_orders!=b.n_orders)return false;
    for(int i=0;i<a.n_units;++i)
        if(a.units[i].op!=b.units[i].op || a.units[i].arg!=b.units[i].arg || a.units[i].n!=b.units[i].n)return false;
    for(int i=0;i<a.n_orders;++i)
        if(a.orders[i].op!=b.orders[i].op || a.orders[i].item!=b.orders[i].item || a.orders[i].n!=b.orders[i].n)return false;
    return true;
}
template<class T> void array(std::ostream& out,const T* values,int n) {
    out<<'[';for(int i=0;i<n;++i){if(i)out<<',';out<<+values[i];}out<<']';
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    const char* opponents[]={"empty_sale_slots_m2","teammate_shoprouter","public_router","public_router_v52",
        "ahmed_v23","junghoon_78","john_131","king_rc4","pass"};
    std::ofstream out(argv[1]);out<<"{\"observations\":[";bool first=true;
    int prefixes=0;
    for(const char* name:opponents)for(int seed=1000;seed<1064;++seed)for(int seat=0;seat<2;++seat) {
        Config config;config.seed=seed;Sim sim(config);
        auto original=make_agent("empty_sale_slots_m2"),rival=make_agent(name);
        compositions_crop_context::empty_sale_slots_m2::Agent trial[3];
        const auto init=agent::runtime::make_agent_init(sim,seat);
        original.reset(init);rival.reset(agent::runtime::make_agent_init(sim,seat^1));
        for(int mode=0;mode<3;++mode){trial[mode].set_crop_mode(mode);trial[mode].reset(init);}
        agent::DecisionBudget budget;budget.max_expansions=100000;
        uint64_t rng=uint64_t(seed)^0xa37108e62d045fb9ULL;
        uint8_t shops[8];for(auto& s:shops)s=random_word(rng)%N_SHOPS;
        int old_market[N_PRODUCTS]{};
        for(int step=0;step<=288;++step) {
            std::copy_n(shops,sim.st.n_shops,sim.st.shops);
            const auto own=agent::runtime::make_observation(sim,seat);
            const auto other=agent::runtime::make_observation(sim,seat^1);
            if(step==264)std::copy_n(own.market.inventory,N_PRODUCTS,old_market);
            Action acts[2],alternative[3];
            original.act(own,budget,acts[seat]);rival.act(other,budget,acts[seat^1]);
            for(int mode=0;mode<3;++mode){
                trial[mode].act(own,budget,alternative[mode]);validate_action(alternative[mode],own);
                if((step<288 || mode==0) && !same(acts[seat],alternative[mode]))return 3;
            }
            if(step==288) {
                if(!first)out<<',';first=false;
                out<<"{\"seed\":"<<seed<<",\"seat\":"<<seat<<",\"opponent\":\""<<name<<"\",\"step\":288,\"shops\":";
                array(out,own.shops,own.n_shops);
                out<<",\"inventory\":";array(out,own.market.inventory,N_PRODUCTS);
                out<<",\"prices\":";array(out,own.market.prices,N_PRODUCTS);
                out<<",\"previous_day_market\":";array(out,old_market,N_PRODUCTS);
                out<<",\"own_cash\":"<<own.self().money<<",\"rival_cash\":"<<own.opponent().money;
                out<<",\"own_shed\":";array(out,own.own.shed,N_ITEMS);
                out<<",\"own_seeds\":";array(out,own.own.seeds,N_CROPS);
                int choices[3];for(int mode=0;mode<3;++mode)choices[mode]=trial[mode].crop_selected();
                out<<",\"tomato_selected\":";array(out,choices,3);
                out<<",\"public_tiles\":[";
                for(int who=0;who<2;++who){
                    if(who)out<<',';out<<'[';
                    const auto& farm=who?own.opponent():own.self();
                    for(int cell=0;cell<100;++cell){
                        if(cell)out<<',';const auto& t=farm.tiles[cell/10][cell%10];
                        const int values[]={t.kind,t.what,t.has_animal,t.planted_day,t.yield_units,t.pending_care_bonus,t.consecutive_dry,t.fertilized_until_day};
                        array(out,values,8);
                    }
                    out<<']';
                }
                out<<"]}";++prefixes;
            } else sim.step(acts[0],acts[1]);
        }
    }
    out<<"],\"common_prefixes\":"<<prefixes<<",\"same_through_step\":287,\"control_same_at_step288\":true}\n";
    return prefixes==1152?0:4;
}
