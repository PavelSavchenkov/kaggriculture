#include "../include/crop_source_season.hpp"

// Semantic donor suffix: leave the current wheat after its normal harvest,
// then tomato d13..24, clear d25, and resume the source carrot d26..29.
// All surrounding tile obligations are retained and every changed day is solved.
struct Rotation {
    int cell=10, first=12, plant=13, clear=25, resume=26;
    uint32_t water=(1u<<13)|(1u<<15)|(1u<<17)|(1u<<19)|(1u<<20)|(1u<<21)|(1u<<22)|(1u<<23);
    uint32_t fertilize=(1u<<20)|(1u<<23);
    uint32_t harvest=(1u<<21)|(1u<<22)|(1u<<23)|(1u<<24);
};

std::vector<TileWorkAction> source_work(const DayProblem& problem,int cell) {
    for(const auto& work:problem.tile_work)if(work.tile==cell)return work.actions;
    return {};
}

std::vector<TileWorkAction> rotation_work(const Rotation& r,int day,const Tile& tile,
                                        const std::vector<TileWorkAction>& original) {
    std::vector<TileWorkAction> work;
    auto add=[&](int op,int arg=-1){TileWorkAction a;a.op=op;a.arg=arg;work.push_back(a);};
    if(day==r.first) {
        for(const auto& a:original) {if(a.op==OP_PLANT)break;work.push_back(a);}
    } else if(day==r.plant) {
        if(tile.kind==T_PLANT && !CROPS[tile.what].ongoing && day-tile.planted_day>=CROPS[tile.what].first_yield_day) {
            add(OP_WATER);add(OP_HARVEST,tile.what);
        } else if(tile.kind!=T_EMPTY)add(OP_DIG);
        add(OP_PLANT,TOMATO);
    } else if(day==r.clear) {
        if(tile.kind!=T_EMPTY)add(OP_DIG);
    } else if(day==r.resume) {
        if(tile.kind!=T_EMPTY)add(OP_DIG);
        add(OP_PLANT,CARROT);add(OP_WATER);
    } else if(day>r.resume)return original;
    if(on(r.fertilize,day))add(OP_FERTILIZE);
    if(on(r.water,day))add(OP_WATER);
    if(on(r.harvest,day))add(OP_HARVEST,TOMATO);
    return work;
}

// Exact isolated tile biology, with unconstrained supplies. This is an offline
// expected-state calculation; the day solver must realize its field actions.
Tile tile_endpoint(const Sim& start,int cell,std::vector<TileWorkAction>& work) {
    auto sim=start;auto& farm=sim.st.farms[0];farm.n_units=1;
    farm.pos_x[0]=cell%10;farm.pos_y[0]=cell/10;
    farm.inv_add(0,FERTILIZER,20);std::fill_n(farm.seeds,N_CROPS,20);
    Action pass;pass.n_units=1;pass.finalize();
    for(auto& job:work) {
        const auto before=farm.produced[job.arg>=0?job.arg:0];
        Action action=pass;action.units[0]={job.op,uint8_t(std::max(0,int(job.arg))),1};action.finalize();
        sim.step(action,pass);
        if(job.op==OP_HARVEST) {
            job.output_item=job.arg;
            job.output_quantity=farm.produced[job.arg]-before;
            if(job.output_quantity<=0)std::abort();
        }
    }
    const bool empty=farm.tiles[cell/10][cell%10].kind==T_EMPTY;
    const int day=sim.st.day;
    while(sim.st.day==day && !sim.st.done)sim.step(pass,pass);
    auto end=sim.st.farms[0].tiles[cell/10][cell%10];
    // Empty-tile stochastic weeds do not belong to a deterministic contract.
    if(end.kind==T_WEED && empty)end={};
    return end;
}

void estimate_rotation(const Season& source,const Rotation& r,const fs::path& file) {
    const auto begin=std::chrono::steady_clock::now();
    Service service{r.water,0,0,0,r.harvest,r.fertilize};
    const auto added=biology({TOMATO,1,r.plant,r.clear},service);
    std::array<int,N_PRODUCTS> delta{};int seed_delta=added.seed_cost,fert_delta=0,operations=1;
    double shadow=-seed_delta;
    for(int day=0;day<30;++day) {
        const auto& d=added.days[day];fert_delta+=d.fertilizer;operations+=d.operations;
        const auto& inv=source.days[day].start.st.market.inventory;
        shadow-=d.fertilizer*market_price(FERTILIZER,inv[FERTILIZER]);
        for(int i=0;i<N_PRODUCTS;++i){delta[i]+=d.output[i];shadow+=d.output[i]*market_price(i,inv[i]);}
    }
    for(const auto& life:source.lives)if(life.cell==r.cell && life.cohort.start_day>=r.first && life.cohort.start_day<r.resume) {
        const auto old=biology(life.cohort,life.service);seed_delta-=old.seed_cost;shadow+=old.seed_cost;
        for(int day=0;day<30;++day) {
            const auto& d=old.days[day];fert_delta-=d.fertilizer;operations-=d.operations;
            const auto& inv=source.days[day].start.st.market.inventory;
            shadow+=d.fertilizer*market_price(FERTILIZER,inv[FERTILIZER]);
            for(int i=0;i<N_PRODUCTS;++i){delta[i]-=d.output[i];shadow-=d.output[i]*market_price(i,inv[i]);}
        }
    }
    std::ofstream out(file);out<<"{\"scope\":\"Offline source-scenario quotes; heuristic excludes endogenous price, labor, funding, storage, unused seed stock and opponent response. Not a runtime forecast.\",\"output_delta\":[";
    for(int i=0;i<N_PRODUCTS;++i)out<<(i?",":"")<<delta[i];
    out<<"],\"seed_cost_delta\":"<<seed_delta<<",\"fertilizer_delta\":"<<fert_delta<<",\"operations_delta\":"<<operations<<",\"shadow_cash_delta\":"<<shadow<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count()<<"}\n";
}

std::array<int,N_ITEMS> flow(const std::vector<TileWorkAction>& work) {
    std::array<int,N_ITEMS> result{};
    for(const auto& job:work) {
        if(job.output_item>=0)result[job.output_item]+=job.output_quantity;
        if(job.op==OP_FERTILIZE)--result[FERTILIZER];
        if(job.op==OP_FEED)--result[WHEAT];
    }
    return result;
}

bool add_order(std::array<Action,24>& actions,int first,int last,int op,int item,int n) {
    for(int h=first;h<=last;++h)if(actions[h].n_orders<10) {
        actions[h].orders[actions[h].n_orders++]={uint8_t(op),uint8_t(item),n};
        actions[h].finalize();return true;
    }
    return false;
}

void market_contract(DayProblem& p,const std::array<Action,24>& actions) {
    p.market_plan.clear();p.sale_targets.clear();p.shed_availability={};p.worker_count=1;
    std::array<int64_t,N_ITEMS> sold{};
    for(int h=0;h<24;++h) {
        std::array<int64_t,N_ITEMS> now{};
        for(int s=0;s<actions[h].n_orders;++s) {
            const auto& order=actions[h].orders[s];
            if(order.op==M_NONE || order.n<=0)continue;
            if(order.op==M_SELL){now[order.item]+=order.n;continue;}
            p.market_plan.push_back({int8_t(h),int8_t(s),order.op,int16_t(order.op==M_HIRE?-1:order.item),order.n});
            if(order.op==M_HIRE)p.worker_count+=order.n;
        }
        for(auto& event:p.market_plan)if(event.hour==h && event.market_op==M_BUY_PRODUCT) {
            const int cancel=std::min<int64_t>(event.quantity,now[event.item]);
            event.quantity-=cancel;now[event.item]-=cancel;
        }
        for(int i=0;i<N_ITEMS;++i){sold[i]+=now[i];p.shed_availability[h][i]=sold[i];}
    }
    std::erase_if(p.market_plan,[](const auto& e){return !e.quantity;});
}

// Reuse a complete route as a bounded compiler alternative. Field jobs on the
// changed tile are assigned to existing visits; movement, other tiles and all
// market commitments remain owned by their original modules. Strict physical
// replay decides whether the altered job queue is actually realizable.
std::optional<std::array<Action,24>> reuse_visits(const RecordedDay& source,
        const DayProblem& problem,const std::array<Action,24>& markets,
        const std::vector<TileWork>& replacements) {
    auto actions=source.own;auto sim=source.start;
    std::array<std::vector<std::pair<int,int>>,100> visits;
    std::array<bool,100> changed{};for(const auto& work:replacements)changed[work.tile]=true;
    for(int h=0;h<24;++h) {
        const auto& farm=sim.st.farms[0];
        for(int u=0;u<farm.n_units;++u) {
            const auto op=actions[h].units[u].op;
            const int cell=farm.pos_y[u]*10+farm.pos_x[u];
            if(changed[cell] &&
               (op==OP_PASS || (op>=OP_PLANT && op<=OP_CARE))) {
                visits[cell].emplace_back(h,u);actions[h].units[u]={};
            }
        }
        sim.step(source.own[h],source.rival[h]);
        actions[h].n_orders=markets[h].n_orders;
        std::copy_n(markets[h].orders,markets[h].n_orders,actions[h].orders);actions[h].finalize();
    }
    for(const auto& work:replacements) {
        if(visits[work.tile].size()<work.actions.size())return {};
        for(size_t i=0;i<work.actions.size();++i) {
            auto [h,u]=visits[work.tile][i];const auto& job=work.actions[i];
            actions[h].units[u]={job.op,uint8_t(std::max(0,int(job.arg))),job.quantity};actions[h].finalize();
        }
    }
    const auto replay=replay_schedule(problem,actions);
    if(replay.requirements_satisfied && replay.invariants_satisfied)return actions;
    return {};
}

bool endpoint_equal(const Farm& actual,const DayProblem& p,int day) {
    for(int i=0;i<N_ITEMS;++i)if(actual.shed[i]!=p.end_shed[i])return false;
    for(int i=0;i<N_CROPS;++i)if(actual.seeds[i]!=p.end_seeds[i])return false;
    for(int cell=0;cell<100;++cell) {
        auto value=managed(actual.tiles[cell/10][cell%10],day);
        const auto expected=*p.required_end_tiles[cell].exact_state;
        if(value.kind==ManagedTileKind::WEED && expected.kind==ManagedTileKind::EMPTY)value={};
        if(value!=expected)return false;
    }
    return true;
}

void export_day(const GuardedDay& component,const std::string& name,const fs::path& folder) {
    fs::create_directories(folder);export_schedule(name,component.plan.actions,folder);
    save_actions(component.plan.actions,folder/"actions.txt");
    std::ofstream state(folder/"guard.txt");state<<component.plan.day<<' '<<component.quadrants<<'\n';
    for(int cell=0;cell<100;++cell){state<<component.check[cell];for(int v:component.tiles[cell])state<<' '<<v;state<<'\n';}
    for(int v:component.shed)state<<v<<' ';state<<'\n';
    for(int v:component.seeds)state<<v<<' ';state<<'\n';
}

int main(int argc,char** argv) {
    if(argc!=5){std::cerr<<"usage: compile_crop_rotation NEW_RUN SEED CELL SECONDS\n";return 2;}
    const fs::path directory=fs::absolute(argv[1]);const uint64_t seed=std::stoull(argv[2]);
    std::vector<Rotation> rotations;std::stringstream cells(argv[3]);std::string value;
    while(std::getline(cells,value,',')){Rotation r;r.cell=std::stoi(value);if(r.cell<0 || r.cell>=100)return 2;rotations.push_back(r);}
    const double seconds=std::stod(argv[4]);
    if(fs::exists(directory) || rotations.empty() || seconds<=0)return 2;
    std::array<bool,100> used{};for(const auto& r:rotations){if(used[r.cell])return 2;used[r.cell]=true;}
    const int first=rotations[0].first;
    fs::create_directories(directory/"days");const auto name=directory.filename().string();
    const Season season(seed);for(const auto& r:rotations)estimate_rotation(season,r,directory/("estimate_"+std::to_string(r.cell)+".json"));
    Config config;config.seed=seed;Sim sim(config);
    Source own;public_router::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    auto step=[&](const Action* override) {
        std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);
        Action pair[2];own.act(agent::runtime::make_observation(sim,0),{},pair[0]);
        rival.act(agent::runtime::make_observation(sim,1),{},pair[1]);
        if(override)pair[0]=*override;
        sim.step(pair[0],pair[1]);
    };
    while(sim.st.day<first)step(nullptr);
    std::vector<GuardedDay> sequence;
    std::ofstream log(directory/"compile.csv");log<<"day,extra_hires,solved,endpoint,seconds\n";
    for(int day=first;day<29;++day) {
        std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);
        const auto& source=season.days[day];RecordedDay actual(sim);
        auto problem=source.problem;problem.start=actual.problem.start;
        auto markets=source.own;std::array<int,N_ITEMS> flow_delta{};std::array<int,N_CROPS> seed_delta{};
        std::vector<TileWork> replacements;
        for(const auto& r:rotations) {
            const auto original=source_work(problem,r.cell);
            auto work=rotation_work(r,day,sim.st.farms[0].tiles[r.cell/10][r.cell%10],original);
            const auto expected=tile_endpoint(sim,r.cell,work);
            std::erase_if(problem.tile_work,[&](const auto& w){return w.tile==r.cell;});
            if(!work.empty())problem.tile_work.push_back({int16_t(r.cell),work});
            replacements.push_back({int16_t(r.cell),work});
            problem.required_end_tiles[r.cell].exact_state=managed(expected,day+1);
            const auto before=flow(original),after=flow(work);
            for(int i=0;i<N_ITEMS;++i)flow_delta[i]+=after[i]-before[i];
            for(const auto& job:original)if(job.op==OP_PLANT)++seed_delta[job.arg];
            for(const auto& job:work)if(job.op==OP_PLANT)--seed_delta[job.arg];
        }
        // Preserve final useful stock. Replace lost feed by fewer wheat sales
        // or an explicit purchase; require surplus crops to be deposited/sold.
        for(int i=0;i<N_ITEMS;++i) {
            int delta=actual.problem.start.shed[i]-source.problem.start.shed[i]+flow_delta[i];
            delta+=season.days[day+1].start.st.farms[0].discarded[i]-source.start.st.farms[0].discarded[i];
            if(delta<0)for(int h=23;h>=0 && delta<0;--h)for(int s=markets[h].n_orders-1;s>=0 && delta<0;--s) {
                auto& order=markets[h].orders[s];if(order.op!=M_SELL || order.item!=i)continue;
                const int remove=std::min(-delta,order.n);order.n-=remove;delta+=remove;
                if(!order.n)order={};markets[h].finalize();
            }
            if(delta<0 && (i!=WHEAT && i!=FERTILIZER)) {std::cerr<<"unbuyable deficit day "<<day<<" item "<<i<<'\n';return 3;}
            if(delta<0 && !add_order(markets,0,22,M_BUY_PRODUCT,i,-delta))return 3;
            if(delta>0 && !add_order(markets,23,23,M_SELL,i,delta))return 3;
        }
        for(int i=0;i<N_CROPS;++i) {
            int delta=actual.problem.start.seeds[i]-source.problem.start.seeds[i]+seed_delta[i];
            problem.end_seeds[i]+=delta;
            if(problem.end_seeds[i]<0) {
                if(!add_order(markets,0,22,M_BUY_SEED,i,-problem.end_seeds[i]))return 3;
                problem.end_seeds[i]=0;
            }
        }
        bool kept=false;
        for(int hires=0;hires<=2 && !kept;++hires) {
            auto trial=problem;auto orders=markets;
            for(int i=0;i<hires;++i)if(!add_order(orders,0,2,M_HIRE,0,1))return 3;
            market_contract(trial,orders);day_scheduler::prepare_problem(trial);
            const auto folder=directory/"days"/std::to_string(day);fs::create_directories(folder);
            save_problem_json(trial,folder/("problem_h"+std::to_string(hires)+".json"));
            day_scheduler::Options budget;budget.seconds=seconds;budget.fallback_workers=1;
            day_scheduler::Result solved;
            if(hires==0)solved.schedule=reuse_visits(source,trial,orders,replacements);
            const bool reused=bool(solved.schedule);
            if(!solved.schedule)solved=day_scheduler::solve(trial,budget);
            bool equal=false;
            if(solved.schedule) {
                auto actions=*solved.schedule;
                for(int h=0;h<24;++h) {
                    actions[h].n_orders=orders[h].n_orders;
                    std::copy_n(orders[h].orders,orders[h].n_orders,actions[h].orders);actions[h].finalize();
                }
                // The full live-opponent day must realize the exact requested
                // endpoint; a merely feasible physical schedule is insufficient.
                const auto saved=sim;auto saved_own=own;auto saved_rival=rival;
                for(int h=0;h<24;++h)step(&actions[h]);
                equal=endpoint_equal(sim.st.farms[0],trial,day+1);
                if(equal) {
                    auto guard=guarded(actual,trial,actions);sequence.push_back(guard);
                    export_day(guard,name+"_d"+std::to_string(day),folder);
                    save_problem_json(trial,folder/"problem.json");kept=true;
                } else {sim=saved;own=saved_own;rival=saved_rival;}
            }
            log<<day<<','<<hires<<','<<bool(solved.schedule)<<','<<equal<<','<<solved.seconds<<'\n';log.flush();
            std::cout<<"day="<<day<<" hires="<<hires<<" reused="<<reused<<" solved="<<bool(solved.schedule)<<" endpoint="<<equal<<std::endl;
        }
        if(!kept){std::ofstream(directory/"STATUS.json")<<"{\"status\":\"compiler_failure\",\"day\":"<<day<<"}\n";return 4;}
    }
    while(!sim.st.done)step(nullptr);
    Options options;options.a=name;options.b="public_router";options.games=32;options.seed_start=1000;
    options.threads=6;options.profile=true;options.validate=true;
    for(int i=0;i<options.games;++i)options.seeds.push_back(options.seed_start+i);
    fs::create_directories(directory/"exact");options.output=(directory/"exact/parent.json").string();
    run_batch(options,[]{return Source{};},[]{return public_router::Agent{};});
    options.output=(directory/"exact/candidate.json").string();
    run_batch(options,[&]{return GuardedSequenceAgent<Source>(sequence);},[]{return public_router::Agent{};});
    std::ofstream(directory/"STATUS.json")<<"{\"status\":\"compiled\",\"days\":"<<sequence.size()<<",\"cash\":"<<sim.st.farms[0].money<<",\"rival_cash\":"<<sim.st.farms[1].money<<"}\n";
}
