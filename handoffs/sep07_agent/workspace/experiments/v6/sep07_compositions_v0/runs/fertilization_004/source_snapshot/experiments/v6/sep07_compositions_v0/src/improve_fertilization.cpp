#include "../include/day_compile_helpers.hpp"
#include "../include/biology.hpp"
#include "../include/guarded_sequence.hpp"
#include "../candidates/investment_context_guarded_001_best/source/agent.hpp"

using namespace compositions;
using namespace compositions::day_contract;
using Source=investment_context_guarded_001_best::Agent;
using Policy=GuardedDayAgent<Source>;

struct CropLife {
    int cell;
    Cohort cohort;
    Service service{0,0,0,0,0,0};
    std::array<int,30> harvested{};
    bool exact=false;
};

struct Season {
    std::vector<RecordedDay> days;
    std::vector<CropLife> lives;
    std::array<uint8_t,8> shops;
    std::array<std::array<Action,24>,30> raw;
    explicit Season(uint64_t seed) {
        Config config;config.seed=seed;Sim sim(config);Source own;public_router::Agent rival;
        own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
        uint64_t random=seed^0xa37108e62d045fb9ULL;
        for(auto& shop:shops)shop=random_word(random)%N_SHOPS;
        days.reserve(30);
        while(!sim.st.done) {
            std::copy_n(shops.begin(),sim.st.n_shops,sim.st.shops);
            if(sim.st.hour==0)days.emplace_back(sim);
            Action pair[2];own.act(agent::runtime::make_observation(sim,0),{},pair[0]);
            rival.act(agent::runtime::make_observation(sim,1),{},pair[1]);
            raw[sim.st.day][sim.st.hour]=pair[0];
            const auto before=sim;sim.step(pair[0],pair[1]);append_contract(days.back(),before,sim,pair);
        }
        std::array<int,100> active;active.fill(-1);
        for(int day=0;day<30;++day) {
            for(const auto& work:days[day].problem.tile_work)for(const auto& op:work.actions) {
                auto& id=active[work.tile];
                if(op.op==OP_PLANT) {
                    if(id>=0)std::abort();
                    id=lives.size();lives.push_back({work.tile,{uint8_t(op.arg),1,day,30}});
                }
                if(id<0)continue;
                auto& life=lives[id];const uint32_t bit=uint32_t(1)<<day;
                if(op.op==OP_WATER)life.service.water|=bit;
                if(op.op==OP_FERTILIZE)life.service.fertilize|=bit;
                if(op.op==OP_HARVEST) {
                    life.service.harvest|=bit;life.harvested[day]+=op.output_quantity;
                    if(!CROPS[life.cohort.item].ongoing){life.cohort.end_day=day+1;id=-1;}
                }
                if(op.op==OP_DIG){life.cohort.end_day=day+1;id=-1;}
            }
            if(day<29)for(int cell=0;cell<100;++cell)if(active[cell]>=0) {
                const auto& tile=days[day+1].start.st.farms[0].tiles[cell/10][cell%10];
                auto& life=lives[active[cell]];
                if(tile.kind!=T_PLANT || tile.planted_day!=life.cohort.start_day) {
                    life.cohort.end_day=day+1;active[cell]=-1;
                }
            }
        }
        for(auto& life:lives) {
            const auto predicted=biology(life.cohort,life.service);life.exact=true;
            for(int day=0;day<30;++day)life.exact&=predicted.days[day].output[life.cohort.item]==life.harvested[day];
        }
    }
};

// Offline closure construction. Rebind source worker routes to the actual
// changed farm, then validate the full branch against a live opponent. This
// does not grant runtime access to a simulator or future shop sequence.
std::vector<GuardedDay> continuation(const Season& season,const GuardedDay& first,int product,uint64_t seed) {
    Config config;config.seed=seed;Sim sim(config);Source own;public_router::Agent rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    std::vector<GuardedDay> result{first};
    while(!sim.st.done) {
        std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);
        const int day=sim.st.day,hour=sim.st.hour;
        if(hour==0 && day>first.plan.day && day<29) {
            auto actions=season.raw[day];
            // Preserve original requested sale quantities and timing. Clearing
            // all crop stock here also moves existing inventory sales and is a
            // separate economic edit, not a route repair.
            RecordedDay boundary(sim);result.push_back(guarded(boundary,season.days[day].problem,actions));
        }
        Action pair[2];own.act(agent::runtime::make_observation(sim,0),{},pair[0]);
        rival.act(agent::runtime::make_observation(sim,1),{},pair[1]);
        if(day>=first.plan.day && day<29) {
            pair[0]=result.back().plan.actions[hour];
            for(int u=pair[0].n_units;u<sim.st.farms[0].n_units;++u)pair[0].units[u]={};
            pair[0].n_units=sim.st.farms[0].n_units;pair[0].finalize();
        }
        sim.step(pair[0],pair[1]);
    }
    return result;
}

struct FertEdit {
    int day,cell,item,extra_output;
    double estimate;
    Tile expected;
};

std::vector<FertEdit> propose(const Season& season) {
    std::vector<FertEdit> result;
    for(const auto& life:season.lives)if(life.exact) {
        const auto& crop=CROPS[life.cohort.item];
        for(int day=std::max(1,life.cohort.start_day+1);day<std::min(29,life.cohort.end_day);++day) {
            const auto& source=season.days[day];const int cell=life.cell;
            const auto& start=source.start.st.farms[0].tiles[cell/10][cell%10];
            const auto& end=season.days[day+1].start.st.farms[0].tiles[cell/10][cell%10];
            if(source.discarded || start.kind!=T_PLANT || start.planted_day!=life.cohort.start_day ||
               start.fertilized_until_day>=day || on(life.service.fertilize,day) ||
               end.kind!=T_PLANT || end.planted_day!=start.planted_day)continue;
            auto service=life.service;service.fertilize|=uint32_t(1)<<day;
            const auto changed=biology(life.cohort,service);
            int extra=0;for(int d=day;d<30;++d)extra+=changed.days[d].output[life.cohort.item]-life.harvested[d];
            if(extra<=0)continue;
            Tile expected=end;expected.fertilized_until_day=day+2;
            const int age=day-start.planted_day;
            if(on(life.service.water,day)) {
                const int since=age+1-crop.first_yield_day;
                const bool grows=crop.ongoing?(since>=0 && since%crop.interval==0 && since/crop.interval+1<=crop.max_yield):
                    (age>=(crop.max_yield_day+1)/2 && age<=crop.max_yield_day);
                if(grows)expected.yield_units=std::min(crop.max_yield,int(expected.yield_units)+1);
            }
            const auto& inventory=source.start.st.market.inventory;
            const double estimate=extra*market_price(life.cohort.item,inventory[life.cohort.item])-market_price(FERTILIZER,inventory[FERTILIZER]);
            result.push_back({day,cell,life.cohort.item,extra,estimate,expected});
        }
    }
    std::stable_sort(result.begin(),result.end(),[](const auto& a,const auto& b){return a.estimate>b.estimate;});
    return result;
}

int main(int argc,char** argv) {
    if(argc!=5){std::cerr<<"usage: improve_fertilization NEW_RUN SEED LIMIT SECONDS\n";return 2;}
    const fs::path directory=fs::absolute(argv[1]);const uint64_t seed=std::stoull(argv[2]);
    const int limit=std::stoi(argv[3]);const double seconds=std::stod(argv[4]);const auto run=directory.filename().string();
    if(fs::exists(directory) || limit<0 || seconds<=0)return 2;
    fs::create_directories(directory/"proposals");fs::create_directories(directory/"exact");
    const Season season(seed);const auto begin=std::chrono::steady_clock::now();const auto edits=propose(season);
    const double estimate_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    std::ofstream biology_log(directory/"biology.csv");biology_log<<"cell,item,start,end,baseline_exact,output\n";
    for(const auto& life:season.lives)biology_log<<life.cell<<','<<int(life.cohort.item)<<','<<life.cohort.start_day<<','<<life.cohort.end_day<<','<<life.exact<<','<<std::accumulate(life.harvested.begin(),life.harvested.end(),0)<<'\n';
    std::ofstream estimates(directory/"estimates.csv");estimates<<"id,day,cell,item,extra_output,estimate\n";
    for(int id=0;id<int(edits.size());++id){const auto& e=edits[id];estimates<<id<<','<<e.day<<','<<e.cell<<','<<e.item<<','<<e.extra_output<<','<<e.estimate<<'\n';}
    estimates.close();biology_log.close();
    std::ofstream(directory/"timing.json")<<"{\"lives\":"<<season.lives.size()<<",\"baseline_exact\":"<<std::count_if(season.lives.begin(),season.lives.end(),[](const auto& l){return l.exact;})<<",\"proposals\":"<<edits.size()<<",\"estimate_seconds\":"<<estimate_seconds<<"}\n";
    std::ofstream log(directory/"compiled.csv");log<<"id,day,cell,item,extra_output,estimate,extra_hires,funded,solved,seconds,endpoint_equal,cash_equal,source_matched\n";
    Options baseline;baseline.a="investment_context_guarded_001_best";baseline.b="public_router";baseline.validate=true;
    baseline.games=32;baseline.seed_start=1000;baseline.threads=8;baseline.profile=true;baseline.output=(directory/"exact/parent.json").string();
    for(int i=0;i<baseline.games;++i)baseline.seeds.push_back(baseline.seed_start+i);
    run_batch(baseline,[]{return Source{};},[]{return public_router::Agent{};});
    for(int id=0;id<std::min(limit,int(edits.size()));++id) {
        const auto& edit=edits[id];const auto& source=season.days[edit.day];bool retained=false;
        for(int extra_hires=0;extra_hires<=1 && !retained;++extra_hires) {
            auto problem=source.problem;auto markets=source.own;
            auto work=std::find_if(problem.tile_work.begin(),problem.tile_work.end(),[&](const auto& w){return w.tile==edit.cell;});
            if(work==problem.tile_work.end()){problem.tile_work.push_back({int16_t(edit.cell),{}});work=problem.tile_work.end()-1;}
            TileWorkAction fertilize;fertilize.op=OP_FERTILIZE;work->actions.insert(work->actions.begin(),fertilize);
            problem.required_end_tiles[edit.cell].exact_state=managed(edit.expected,edit.day+1);
            bool funded=insert_funded_order(source,problem,markets,season.shops,M_BUY_PRODUCT,FERTILIZER,1);
            if(extra_hires && funded){funded=insert_funded_order(source,problem,markets,season.shops,M_HIRE,0,1);++problem.worker_count;}
            const auto name=run+"_"+std::to_string(id);const auto folder=directory/"proposals"/name;
            fs::create_directories(folder);
            day_scheduler::Result solved;bool equal=false,cash_equal=false,matched=false;
            if(funded) {
                day_scheduler::prepare_problem(problem);save_problem_json(problem,folder/("problem_h"+std::to_string(extra_hires)+".json"));
                day_scheduler::Options budget;budget.seconds=seconds;budget.fallback_workers=1;solved=day_scheduler::solve(problem,budget);
            }
            if(solved.schedule) {
                auto actions=*solved.schedule;auto rebuilt=source.start;auto financial=source.start;
                for(int hour=0;hour<24;++hour) {
                    std::copy_n(season.shops.begin(),rebuilt.st.n_shops,rebuilt.st.shops);
                    std::copy_n(season.shops.begin(),financial.st.n_shops,financial.st.shops);
                    actions[hour].n_orders=markets[hour].n_orders;
                    std::copy_n(markets[hour].orders,markets[hour].n_orders,actions[hour].orders);actions[hour].finalize();
                    rebuilt.step(actions[hour],source.rival[hour]);financial.step(markets[hour],source.rival[hour]);
                }
                const auto& farm=rebuilt.st.farms[0];const auto& expected=season.days[edit.day+1].start.st.farms[0];
                equal=farm.n_quadrants==expected.n_quadrants;
                for(int i=0;i<N_ITEMS;++i)equal&=farm.shed[i]==expected.shed[i] && farm.produced[i]==expected.produced[i] && farm.discarded[i]==expected.discarded[i];
                for(int i=0;i<N_CROPS;++i)equal&=farm.seeds[i]==expected.seeds[i];
                for(int cell=0;cell<100;++cell) {
                    auto actual=managed(farm.tiles[cell/10][cell%10],edit.day+1);const auto desired=*problem.required_end_tiles[cell].exact_state;
                    if(desired.kind==ManagedTileKind::EMPTY && actual.kind==ManagedTileKind::WEED)actual={};
                    equal&=actual==desired;
                }
                cash_equal=farm.money==financial.st.farms[0].money && rebuilt.st.farms[1].money==financial.st.farms[1].money;
                save_actions(actions,folder/("schedule_h"+std::to_string(extra_hires)+".txt"));
                if(equal && cash_equal) {
                    const auto guard=guarded(source,problem,actions);Policy policy({guard});public_router::Agent rival;
                    Options check;check.validate=true;const auto outcome=run_game(policy,rival,seed,0,check);
                    matched=bool(policy.matched_days()&(uint32_t(1)<<edit.day));
                    if(!matched)std::abort();
                    save_problem_json(problem,folder/"problem.json");save_actions(actions,folder/"combined_schedule.txt");export_schedule(name,actions,folder);
                    auto exact=baseline;exact.a=name;exact.output=(directory/"exact"/(name+".json")).string();
                    run_batch(exact,[&]{return Policy({guard});},[]{return public_router::Agent{};});retained=true;
                    const auto sequence=continuation(season,guard,edit.item,seed);
                    GuardedSequenceAgent<Source> checked_sequence(sequence);public_router::Agent checked_rival;
                    const auto checked_outcome=run_game(checked_sequence,checked_rival,seed,0,check);
                    uint32_t expected_mask=0;for(const auto& component:sequence)expected_mask|=uint32_t(1)<<component.plan.day;
                    if(checked_sequence.matched_days()!=expected_mask)std::abort();
                    std::ofstream(folder/"closure_source.json")<<"{\"matched_days\":"<<expected_mask<<",\"cash\":"<<checked_outcome.cash[0]<<",\"opponent_cash\":"<<checked_outcome.cash[1]<<"}\n";
                    exact.a=name+"_closure";exact.output=(directory/"exact"/(exact.a+".json")).string();
                    run_batch(exact,[&]{return GuardedSequenceAgent<Source>(sequence);},[]{return public_router::Agent{};});
                    const auto closure=folder/"closure";fs::create_directories(closure);
                    for(const auto& component:sequence) {
                        const auto day_folder=closure/std::to_string(component.plan.day);fs::create_directories(day_folder);
                        export_schedule(name+"_d"+std::to_string(component.plan.day),component.plan.actions,day_folder);
                        std::ofstream state(day_folder/"guard.txt");state<<component.plan.day<<' '<<component.quadrants<<'\n';
                        for(int cell=0;cell<100;++cell){state<<component.check[cell];for(int v:component.tiles[cell])state<<' '<<v;state<<'\n';}
                        for(int v:component.shed)state<<v<<' ';state<<'\n';for(int v:component.seeds)state<<v<<' ';state<<'\n';
                    }
                }
            }
            log<<id<<','<<edit.day<<','<<edit.cell<<','<<edit.item<<','<<edit.extra_output<<','<<edit.estimate<<','<<extra_hires<<','<<funded<<','<<bool(solved.schedule)<<','<<solved.seconds<<','<<equal<<','<<cash_equal<<','<<matched<<'\n';log.flush();
            std::cout<<"id="<<id<<" day="<<edit.day<<" cell="<<edit.cell<<" hire="<<extra_hires<<" funded="<<funded<<" solved="<<bool(solved.schedule)<<" endpoint="<<equal<<" cash="<<cash_equal<<std::endl;
        }
    }
}
