// Revalidate a solved final suffix in the complete live-opponent game.
#include "season_v2.hpp"

std::array<Action,24> read_actions(const fs::path& path){
    std::ifstream in(path);if(!in)std::abort();std::array<Action,24> result;
    for(auto& a:result){
        in>>a.n_units>>a.n_orders;
        for(int u=0;u<a.n_units;++u){int op,arg;in>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
        for(int s=0;s<a.n_orders;++s){int op,item;in>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
        a.finalize();
    }
    if(!in)std::abort();return result;
}

bool equal_endpoint(const Farm& farm,const DayProblem& problem,int day){
    for(int i=0;i<N_ITEMS;++i)if(farm.shed[i]!=problem.end_shed[i])return false;
    for(int i=0;i<N_CROPS;++i)if(farm.seeds[i]!=problem.end_seeds[i])return false;
    for(int cell=0;cell<100;++cell){
        auto state=managed(farm.tiles[cell/10][cell%10],day);
        const auto expected=*problem.required_end_tiles[cell].exact_state;
        if(state.kind==ManagedTileKind::WEED && expected.kind==ManagedTileKind::EMPTY)state={};
        if(state!=expected)return false;
    }
    return true;
}

int main(int argc,char** argv){
    if(argc!=7 || fs::exists(argv[6]))return 2;
    const fs::path prefix=argv[1],final_path=argv[2],problem_path=argv[3],orders_path=argv[4],out=argv[6];
    const uint64_t seed=std::stoull(argv[5]);
    const Season season(seed);
    const auto final_problem=load_problem_json(problem_path);
    auto final=read_actions(final_path);
    const auto physical=replay_schedule(final_problem,final);
    if(!physical.requirements_satisfied || !physical.invariants_satisfied || !physical.errors.empty())return 3;
    const auto orders=read_actions(orders_path);
    for(int h=0;h<24;++h){
        final[h].n_orders=orders[h].n_orders;
        std::copy_n(orders[h].orders,orders[h].n_orders,final[h].orders);final[h].finalize();
    }
    fs::create_directories(out/"days");
    Config config;config.seed=seed;Sim sim(config);Source own;public_router::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    std::ofstream progress(out/"daily.csv");
    progress<<"day,cash,rival_cash,parent_cash,parent_rival_cash,hire_cost,parent_hire_cost,exact\n";
    DetailedProfile profile;int accepted_days=0;
    for(int day=0;day<30;++day){
        std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);
        const auto folder=prefix/"days"/std::to_string(day);
        const bool changed=day==29 || fs::exists(folder/"actions.txt");
        const auto plan=day==29?final:changed?read_actions(folder/"actions.txt"):std::array<Action,24>{};
        const auto problem=day==29?final_problem:changed?load_problem_json(folder/"problem.json"):season.days[day].problem;
        const RecordedDay start(sim);
        const int hire_cost_before=profile.farms[0].hire_cost;
        for(int h=0;h<24 && !sim.st.done;++h){
            std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);
            Action actions[2];own.act(agent::runtime::make_observation(sim,0),decision_budget(),actions[0]);
            rival.act(agent::runtime::make_observation(sim,1),decision_budget(),actions[1]);
            if(changed)actions[0]=plan[h];
            for(int p=0;p<2;++p)validate_action(actions[p],agent::runtime::make_observation(sim,p));
            const auto before=sim;sim.step(actions[0],actions[1]);profile.observe(before,sim,actions);
        }
        auto endpoint=sim;
        if(day==29){
            endpoint.st.done=false;endpoint.cfg.episode_steps=744;
            Action padding[2];for(int p=0;p<2;++p){padding[p].n_units=endpoint.st.farms[p].n_units;padding[p].finalize();}
            endpoint.step(padding[0],padding[1]);
        }
        const bool equal=equal_endpoint(endpoint.st.farms[0],problem,day+1);
        const auto parent_cash=day<29?season.days[day+1].start.st.farms[0].money:season.final_farms[0].money;
        const auto parent_rival=day<29?season.days[day+1].start.st.farms[1].money:season.final_farms[1].money;
        int parent_hires=season.days[day].problem.worker_count-1,a=1,b=1,parent_cost=0;
        for(int n=0;n<parent_hires;++n){parent_cost+=a;const int next=a+b;a=b;b=next;}
        progress<<day<<','<<sim.st.farms[0].money<<','<<sim.st.farms[1].money<<','<<parent_cash<<','<<parent_rival
            <<','<<profile.farms[0].hire_cost-hire_cost_before<<','<<parent_cost<<','<<equal<<'\n';progress.flush();
        if(!equal){std::cerr<<"endpoint failure day="<<day<<'\n';return 4;}
        if(changed){
            ++accepted_days;const auto destination=out/"days"/std::to_string(day);
            fs::create_directories(destination);
            const auto component=guarded(start,problem,plan);
            export_schedule("completed_"+std::to_string(seed)+"_d"+std::to_string(day),plan,destination);
            save_actions(plan,destination/"actions.txt");save_problem_json(problem,destination/"problem.json");
            std::ofstream guard(destination/"guard.txt");guard<<component.plan.day<<' '<<component.quadrants<<'\n';
            for(int cell=0;cell<100;++cell){guard<<component.check[cell];for(int x:component.tiles[cell])guard<<' '<<x;guard<<'\n';}
            for(int x:component.shed)guard<<x<<' ';guard<<'\n';for(int x:component.seeds)guard<<x<<' ';guard<<'\n';
        }
    }
    if(sim.st.step!=719 || !sim.st.done)std::abort();
    std::ofstream result(out/"MATCHED_RESULT.json");
    result<<"{\"scope\":\"Complete719-turn fixed-calendar matched world; not deployable policy evaluation\",\"seed\":"<<seed
        <<",\"days\":"<<accepted_days<<",\"parent_cash\":"<<season.final_farms[0].money<<",\"parent_rival_cash\":"<<season.final_farms[1].money
        <<",\"cash\":"<<sim.st.farms[0].money<<",\"rival_cash\":"<<sim.st.farms[1].money
        <<",\"hires\":"<<profile.farms[0].hires<<",\"hire_cost\":"<<profile.farms[0].hire_cost;
    for(int p=0;p<2;++p){
        result<<",\"produced"<<p<<"\":[";
        for(int i=0;i<N_ITEMS;++i)result<<(i?",":"")<<sim.st.farms[p].produced[i];
        result<<"],\"parent_produced"<<p<<"\":[";
        for(int i=0;i<N_ITEMS;++i)result<<(i?",":"")<<season.final_farms[p].produced[i];
        result<<"]";
    }
    result<<"}\n";
    std::ofstream(out/"STATUS.json")<<"{\"status\":\"full_game_and_all_day_endpoints_pass\",\"turns\":719}\n";
    std::cout<<"complete cash="<<sim.st.farms[0].money<<" parent="<<season.final_farms[0].money<<std::endl;
}
