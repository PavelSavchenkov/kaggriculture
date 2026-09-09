// Offline full-game funding/placement diagnosis. No playable policy sees Sim.
#include "experiments/v6/sep07_compositions_v0/runs/animal_groups_sep08_001/source/season_v2.hpp"

std::array<Action,24> load_actions(const fs::path& path){
    std::ifstream in(path);if(!in)std::abort();std::array<Action,24> result;
    for(auto& a:result){
        in>>a.n_units>>a.n_orders;
        for(int u=0;u<a.n_units;++u){int op,arg;in>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
        for(int s=0;s<a.n_orders;++s){int op,item;in>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
        a.finalize();
    }
    if(!in)std::abort();return result;
}

bool endpoint_equal(const Farm& farm,const DayProblem& problem){
    for(int i=0;i<N_ITEMS;++i)if(farm.shed[i]!=problem.end_shed[i])return false;
    for(int i=0;i<N_CROPS;++i)if(farm.seeds[i]!=problem.end_seeds[i])return false;
    for(int cell=0;cell<100;++cell){
        auto actual=managed(farm.tiles[cell/10][cell%10],11);
        const auto expected=*problem.required_end_tiles[cell].exact_state;
        if(actual.kind==ManagedTileKind::WEED && expected.kind==ManagedTileKind::EMPTY)actual={};
        if(actual!=expected)return false;
    }
    return true;
}

int main(int argc,char** argv){
    if(argc!=2 || fs::exists(argv[1]))return 2;const fs::path out=argv[1];fs::create_directories(out);
    const fs::path prior="experiments/v6/sep07_compositions_v0/runs/animal_forecast_error_sep08_001";
    const auto prefix=load_actions(prior/"mixed_certificate/days/9/actions.txt");
    const auto original=load_actions(prior/"mixed_season_30s/days/10/executable_h1.txt");
    const auto raw=load_actions(prior/"mixed_season_30s/days/10/raw_schedule_h1.txt");
    const auto problem=load_problem_json(prior/"mixed_season_30s/days/10/problem_h1.json");
    const Season season(1008);
    std::ofstream market(out/"markets.csv"),units(out/"units.csv"),summary(out/"SUMMARY.csv");
    market<<"late_goose_hour,hour,slot,cash_before,cash_after,op,item,requested,actual_quantity,fixed_paid\n";
    units<<"late_goose_hour,hour,worker,tile,op,item,carry_goose,carry_wheat,tile_kind,animal_before,animal_after,feed_after,care_after\n";
    summary<<"late_goose_hour,cash,wheat,goose14,care_bank14,endpoint,physical\n";
    for(int late:{-1,10,12}){
        auto plan=original,physical=raw;auto contract=problem;
        if(late>=0){
            for(auto* actions:{&plan,&physical}){
                int changed=0;
                for(int slot=0;slot<(*actions)[1].n_orders;++slot){auto& m=(*actions)[1].orders[slot];
                    if(m.op==M_BUY_ANIMAL && m.item==GOOSE){if(m.n!=2)std::abort();m.n=1;++changed;}}
                if(changed!=1 || (*actions)[late].n_orders>=10)std::abort();
                auto& a=(*actions)[late];a.orders[a.n_orders++]={M_BUY_ANIMAL,GOOSE,1};a.finalize();
                (*actions)[1].finalize();
            }
            int changed=0;for(auto& event:contract.market_plan)
                if(event.hour==1 && event.market_op==M_BUY_ANIMAL && event.item==GOOSE){
                    if(event.quantity!=2)std::abort();event.quantity=1;++changed;}
            if(changed!=1)std::abort();
            contract.market_plan.push_back({int8_t(late),int8_t(physical[late].n_orders-1),M_BUY_ANIMAL,GOOSE,1});
            day_scheduler::prepare_problem(contract);
        }
        const auto certificate=replay_schedule(contract,physical);
        const bool valid=certificate.requirements_satisfied && certificate.invariants_satisfied && certificate.errors.empty();
        Config config;config.seed=1008;Sim sim(config);Source own;public_router::Agent rival;
        own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
        while(sim.st.day<11){
            std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);Action actions[2];
            own.act(agent::runtime::make_observation(sim,0),decision_budget(),actions[0]);
            rival.act(agent::runtime::make_observation(sim,1),decision_budget(),actions[1]);
            if(sim.st.day==9)actions[0]=prefix[sim.st.hour];
            if(sim.st.day==10)actions[0]=plan[sim.st.hour];
            for(int p=0;p<2;++p)validate_action(actions[p],agent::runtime::make_observation(sim,p));
            const auto before=sim;const auto accepted=accepted_market(before,actions);sim.step(actions[0],actions[1]);
            if(before.st.day!=10)continue;
            const auto& f=before.st.farms[0];const auto& after=sim.st.farms[0];
            for(int slot=0;slot<actions[0].n_orders;++slot){const auto& m=actions[0].orders[slot];if(m.op==M_NONE)continue;
                const auto& actual=accepted.slots[slot];
                market<<late<<','<<before.st.hour<<','<<slot<<','<<f.money<<','<<after.money<<','<<+m.op<<','<<+m.item
                    <<','<<m.n<<','<<actual.trades[0].n<<','<<actual.fixed[0]<<'\n';
            }
            for(int u=0;u<f.n_units;++u){const auto& a=actions[0].units[u];
                const int cell=f.pos_y[u]*10+f.pos_x[u];const auto& t=f.tiles[cell/10][cell%10];const auto& end=after.tiles[cell/10][cell%10];
                if(a.op!=OP_PICKUP && a.op!=OP_PLACE && a.op!=OP_FEED && a.op!=OP_CARE)continue;
                units<<late<<','<<before.st.hour<<','<<u<<','<<cell<<','<<+a.op<<','<<+a.arg<<','<<f.inv[u][GOOSE]<<','<<f.inv[u][WHEAT]
                    <<','<<+t.kind<<','<<(t.has_animal?int(t.what):-1)<<','<<(end.has_animal?int(end.what):-1)<<','<<end.fed_today<<','<<end.cared_today<<'\n';
            }
        }
        const auto& f=sim.st.farms[0];const auto& t=f.tiles[1][4];const bool exact=endpoint_equal(f,contract);
        summary<<late<<','<<f.money<<','<<f.shed[WHEAT]<<','<<(t.has_animal?int(t.what):-1)<<','<<+t.pending_care_bonus<<','<<exact<<','<<valid<<'\n';
        if(late>=0 && exact && valid){
            const auto dest=out/("hour"+std::to_string(late))/"days/10";fs::create_directories(dest);
            save_actions(plan,dest/"actions.txt");save_actions(physical,dest/"raw_schedule.txt");
            save_problem_json(contract,dest/"problem.json");
            std::ofstream(dest/"STATUS.json")<<"{\"physical_and_live_endpoint\":true}\n";
        }
    }
}
