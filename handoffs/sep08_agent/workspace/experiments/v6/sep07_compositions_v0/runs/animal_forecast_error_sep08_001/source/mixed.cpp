// Offline diagnosis of partial input purchases in an otherwise solved day.
#include "experiments/v6/sep07_compositions_v0/runs/animal_groups_sep08_001/source/season_v2.hpp"
std::array<Action,24> load(const fs::path& path){
    std::ifstream in(path);if(!in)std::abort();std::array<Action,24> result;
    for(auto& a:result){
        in>>a.n_units>>a.n_orders;
        for(int u=0;u<a.n_units;++u){int op,arg;in>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
        for(int s=0;s<a.n_orders;++s){int op,item;in>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
        a.finalize();
    }
    if(!in)std::abort();return result;
}
int main(int argc,char** argv){
    if(argc!=2 || fs::exists(argv[1]))return 2;const fs::path out=argv[1];fs::create_directories(out);
    const fs::path folder="experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/flexible_mixed_30s/mixed_d9/days/9";
    const auto original=load(folder/"executable_h2.txt");const auto problem=load_problem_json(folder/"problem_h2.json");
    const Season season(1008);std::ofstream csv(out/"TRANSACTIONS.csv"),results(out/"RESULTS.csv");
    csv<<"extra_late_wheat,hour,cash_before,cash_after,wheat_price,requested_buys,actual_buys,requested_sales,actual_sales,shed_before,shed_after\n";
    results<<"extra_late_wheat,day_end_cash,wheat,exact_shed_and_seeds,tiles_equal_original\n";
    std::optional<Farm> baseline;
    for(int extra=0;extra<=2;extra+=2){
        auto plan=original;
        if(extra){auto& a=plan[23];if(a.n_orders>=10)std::abort();a.orders[a.n_orders++]={M_BUY_PRODUCT,WHEAT,extra};a.finalize();}
        Config config;config.seed=1008;Sim sim(config);Source own;public_router::Agent rival;DetailedProfile profile;
        own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
        while(sim.st.day<10){
            std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);Action actions[2];
            own.act(agent::runtime::make_observation(sim,0),decision_budget(),actions[0]);
            rival.act(agent::runtime::make_observation(sim,1),decision_budget(),actions[1]);
            if(sim.st.day==9)actions[0]=plan[sim.st.hour];
            for(int p=0;p<2;++p)validate_action(actions[p],agent::runtime::make_observation(sim,p));
            const auto before=sim;const int bought=profile.farms[0].buys[WHEAT];sim.step(actions[0],actions[1]);profile.observe(before,sim,actions);
            if(before.st.day==9){
                int buys=0,sells=0;for(int s=0;s<actions[0].n_orders;++s){const auto& m=actions[0].orders[s];
                    if(m.item==WHEAT && m.op==M_BUY_PRODUCT)buys+=m.n;if(m.item==WHEAT && m.op==M_SELL)sells+=m.n;}
                csv<<extra<<','<<before.st.hour<<','<<before.st.farms[0].money<<','<<sim.st.farms[0].money<<','<<before.st.market.prices[WHEAT]
                    <<','<<buys<<','<<profile.farms[0].buys[WHEAT]-bought<<','<<sells<<','<<sim.st.farms[0].sold_units[WHEAT]-before.st.farms[0].sold_units[WHEAT]
                    <<','<<before.st.farms[0].shed[WHEAT]<<','<<sim.st.farms[0].shed[WHEAT]<<'\n';
            }
        }
        const auto& f=sim.st.farms[0];bool exact=true,tiles=true;
        for(int i=0;i<N_ITEMS;++i)exact&=f.shed[i]==problem.end_shed[i];
        for(int i=0;i<N_CROPS;++i)exact&=f.seeds[i]==problem.end_seeds[i];
        if(!extra)baseline=f;
        else for(int c=0;c<100;++c)tiles&=tile_key(f.tiles[c/10][c%10],10)==tile_key(baseline->tiles[c/10][c%10],10);
        results<<extra<<','<<f.money<<','<<f.shed[WHEAT]<<','<<exact<<','<<tiles<<'\n';
        if(extra && exact && tiles)save_actions(plan,out/"actions.txt");
    }
}
