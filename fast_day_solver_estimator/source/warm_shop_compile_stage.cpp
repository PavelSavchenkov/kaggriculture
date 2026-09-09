#include "warm_shop_season.hpp"
#include "context_model.hpp"
#include <chrono>
#include <iomanip>
#include "baselines/warm/include/crop_service.hpp"
#include "baselines/warm/runs/animal_groups_sep08_001/source/fixed_repair.hpp"
// Offline fixtures only: no cross-scenario policy is claimed here.


double warm_trial_probability(const DayProblem& p, int hours) {
    std::vector<std::pair<int, int>> fixed;
    for (const auto& e : p.market_plan) if (e.market_op == M_HIRE) fixed.emplace_back(e.hour, e.order_index);
    std::sort(fixed.begin(), fixed.end());
    if (int(fixed.size()) + 1 != p.worker_count) throw std::runtime_error("hire count mismatch");
    const auto menu = labor::fixed_planning_menu(p, hours, fixed, {});
    const auto f = labor::extract_context(p, menu);
    return labor::context_model::boost(labor::context_query_features(f, p.worker_count));
}

struct Rotation { int cell=22, first=17, item=GOOSE; };

std::vector<TileWorkAction> source_work(const DayProblem& problem,int cell) {
    for(const auto& work:problem.tile_work)if(work.tile==cell)return work.actions;
    return {};
}

// Every candidate supplies its own dated productive service. This is a
// compiler input, not a rule that every service action is always profitable.
std::vector<TileWorkAction> rotation_work(const Rotation& r,int day,const Tile& tile,
        const std::vector<TileWorkAction>& original) {
    if(day<r.first)return original;
    std::vector<TileWorkAction> result;
    auto add=[&](int op,int arg=-1){TileWorkAction job;job.op=op;job.arg=arg;result.push_back(job);};
    if(is_animal(r.item)) {
        if(day==r.first){
            if(tile.kind==T_WEED)add(OP_DIG);
            else if(tile.kind!=T_EMPTY && tile.kind!=T_LOCKED)std::abort();
            add(r.item==GOOSE?OP_BUILD_COOP:OP_BUILD_PASTURE);
            add(OP_PLACE,r.item);
        }
        const Cohort cohort{uint8_t(r.item),1,r.first,30};
        const auto service=productive_service(cohort);
        if(on(service.feed,day))add(OP_FEED);
        if(on(service.care,day))add(OP_CARE);
        if(tile.fertilizer_available)add(OP_COLLECT_FERTILIZER);
        if(tile.yield_units>0)add(OP_HARVEST,r.item==GOOSE?EGG:r.item==COW?MILK:WOOL);
        return result;
    }
    const int last=earliest_full_crop_age(r.item,true),duration=last+1;
    const int first=r.first+((day-r.first)/duration)*duration;
    if(first+CROPS[r.item].first_yield_day>=30)return result; // Require some real output before the last action.
    const int end=std::min(30,first+duration),harvest=end-1;
    const int age=day-first;
    Cohort cohort{uint8_t(r.item),1,first,end};
    auto service=productive_service(cohort,true);
    if(!CROPS[r.item].ongoing)service.harvest=uint32_t(1)<<harvest;
    // Preserve the donor tomato's two full-capacity harvests when it fits.
    if(r.item==TOMATO){
        service.harvest=0;for(int a:{9,11})if(first+a<end)service.harvest|=uint32_t(1)<<(first+a);
        if(end<first+duration)service.harvest|=uint32_t(1)<<harvest;
    }
    service=prune_crop_service(cohort,service);
    if(age==0){
        if(tile.kind==T_WEED)add(OP_DIG);
        else if(tile.kind!=T_EMPTY && tile.kind!=T_LOCKED)std::abort();
        add(OP_PLANT,r.item);
    }
    if(on(service.fertilize,day))add(OP_FERTILIZE);
    if(on(service.water,day))add(OP_WATER);
    if(on(service.harvest,day) && age>=CROPS[r.item].first_yield_day && (!CROPS[r.item].ongoing || tile.yield_units>0))
        add(OP_HARVEST,r.item);
    if(day==harvest && day<29 && CROPS[r.item].ongoing)add(OP_DIG); // No post-game reuse to justify final clearing.
    return result;
}

Tile tile_endpoint(const Sim& start,int cell,std::vector<TileWorkAction>& work) {
    auto sim=start;sim.cfg.episode_steps=744;
    auto& farm=sim.st.farms[0];farm.n_units=1;
    farm.pos_x[0]=cell%10;farm.pos_y[0]=cell/10;
    for(int i=0;i<N_ITEMS;++i)farm.inv_add(0,i,20);
    std::fill_n(farm.seeds,N_CROPS,20);
    Action pass;pass.n_units=1;pass.finalize();
    if(farm.tiles[cell/10][cell%10].kind==T_LOCKED){
        // Biological endpoint assumes the declared market land purchase succeeds.
        // The actual DayProblem still starts locked and requires that event.
        farm.money+=1000000;Action buy=pass;buy.n_orders=1;buy.orders[0]={M_BUY_LAND,0,1};buy.finalize();
        sim.step(buy,pass);
        if(farm.tiles[cell/10][cell%10].kind!=T_EMPTY)std::abort();
    }

    for(auto& job:work) {
        const auto before=farm.produced[job.arg>=0?job.arg:0];
        Action action=pass;action.units[0]={job.op,uint8_t(std::max(0,int(job.arg))),1};action.finalize();
        sim.step(action,pass);
        if(job.op==OP_HARVEST) {
            job.output_item=job.arg;job.output_quantity=farm.produced[job.arg]-before;
            if(job.output_quantity<=0){std::cerr<<"zero harvest day="<<start.st.day<<" cell="<<cell<<" item="<<job.arg<<std::endl;std::abort();}
        }
        if(job.op==OP_COLLECT_FERTILIZER){job.output_item=FERTILIZER;job.output_quantity=1;}
    }
    const int day=sim.st.day;
    while(sim.st.day==day && !sim.st.done)sim.step(pass,pass);
    return farm.tiles[cell/10][cell%10];
}

std::array<int,N_ITEMS> flow(const std::vector<TileWorkAction>& work) {
    std::array<int,N_ITEMS> result{};
    for(const auto& job:work) {
        if(job.output_item>=0)result[job.output_item]+=job.output_quantity;
        if(job.op==OP_FERTILIZE)--result[FERTILIZER];
        if(job.op==OP_FEED)--result[WHEAT];
        if(job.op==OP_PLACE && is_animal(job.arg))--result[job.arg];
    }
    return result;
}

bool add_order(std::array<Action,24>& actions,int first,int last,int op,int item,int n) {
    for(int h=first;h<=last;++h)if(actions[h].n_orders<10) {
        actions[h].orders[actions[h].n_orders++]={uint8_t(op),uint8_t(item),n};
        actions[h].finalize();return true;
    }
    // Accepted source actions retain empty slots. Reuse those before
    // rejecting a new sale, then merge a same-product sale if the hour is full.
    for(int h=first;h<=last;++h)for(int s=0;s<actions[h].n_orders;++s)
        if(actions[h].orders[s].op==M_NONE) {
            actions[h].orders[s]={uint8_t(op),uint8_t(item),n};actions[h].finalize();return true;
        }
    if(op==M_SELL)for(int h=first;h<=last;++h)for(int s=0;s<actions[h].n_orders;++s) {
        auto& order=actions[h].orders[s];
        if(order.op==M_SELL && order.item==item){order.n+=n;actions[h].finalize();return true;}
    }
    std::cerr<<"no market slot: hours="<<first<<".."<<last<<" operation="<<op<<" item="<<item<<" quantity="<<n<<'\n';
    return false;
}

void market_contract(DayProblem& p,const std::array<Action,24>& actions,int next_quadrant) {
    p.market_plan.clear();p.sale_targets.clear();p.shed_availability={};p.worker_count=1;
    std::array<int64_t,N_ITEMS> sold{};
    for(int h=0;h<24;++h) {
        std::array<int64_t,N_ITEMS> now{};
        for(int s=0;s<actions[h].n_orders;++s) {
            const auto& order=actions[h].orders[s];
            if(order.op==M_NONE)continue;
            const bool fixed=order.op==M_HIRE || order.op==M_BUY_LAND;
            if(!fixed && order.n<=0)continue;
            if(order.op==M_SELL){now[order.item]+=order.n;continue;}
            const int item=order.op==M_HIRE?-1:order.op==M_BUY_LAND?next_quadrant++:order.item;
            p.market_plan.push_back({int8_t(h),int8_t(s),order.op,int16_t(item),fixed?1:order.n});
            if(order.op==M_HIRE)++p.worker_count;
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
    if(argc!=5 && argc!=6)return 2;
    const fs::path resume=argc==6?fs::absolute(argv[5]):fs::path{};
    const fs::path directory=fs::absolute(argv[1]);
    const uint64_t seed=std::stoull(argv[2]);
    const double seconds=std::stod(argv[4]);
    std::ifstream spec(argv[3]);if(!spec || fs::exists(directory) || seconds<=0)return 2;
    std::vector<Rotation> rotations;Rotation r;int first=30;std::array<bool,100> used{};
    while(spec>>r.cell>>r.first>>r.item){
        if(r.cell<0 || r.cell>=100 || r.first<8 || r.first>22 || (!(is_crop(r.item) || is_animal(r.item)) || r.cell%10<5 || r.cell/10<5) || used[r.cell])return 2;
        used[r.cell]=true;first=std::min(first,r.first);rotations.push_back(r);
    }
    if(!spec.eof() || rotations.empty())return 2;
    auto read_actions=[](const fs::path& path){
        std::ifstream input(path);if(!input)std::abort();std::array<Action,24> result;
        for(auto& a:result){
            input>>a.n_units>>a.n_orders;
            for(int u=0;u<a.n_units;++u){int op,arg;input>>op>>arg>>a.units[u].n;a.units[u].op=op;a.units[u].arg=arg;}
            for(int s=0;s<a.n_orders;++s){int op,item;input>>op>>item>>a.orders[s].n;a.orders[s].op=op;a.orders[s].item=item;}
            a.finalize();
        }
        if(!input)std::abort();return result;
    };
    const bool cancel_unused_seeds=true,next_day_tomato_sales=false;
    fs::create_directories(directory/"days");const auto name=directory.filename().string();
    const Season season(seed);
    Config config;config.seed=seed;Sim sim(config);
    Source own;ContextRival rival;
    own.reset(agent::runtime::make_agent_init(sim,0));rival.reset(agent::runtime::make_agent_init(sim,1));
    auto step=[&](const Action* override) {
        std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);
        Action pair[2];own.act(agent::runtime::make_observation(sim,0),decision_budget(),pair[0]);
        rival.act(agent::runtime::make_observation(sim,1),decision_budget(),pair[1]);
        if(override)pair[0]=*override;
        sim.step(pair[0],pair[1]);
    };
    while(sim.st.day<first)step(nullptr);
    std::vector<GuardedDay> sequence;
    std::ofstream guidance(directory/"guidance.csv");guidance<<"day,extra_hires,retry,predicted_success,deferred,prediction_wall_seconds\n"<<std::setprecision(17);
    std::ofstream log(directory/"compile.csv");log<<"day,extra_hires,solved,endpoint,seconds\n";
    for(int day=first;day<30;++day) {
        std::copy_n(season.shops.begin(),sim.st.n_shops,sim.st.shops);
        const auto& source=season.days[day];RecordedDay actual(sim);
        if(!resume.empty() && day<29 && fs::is_regular_file(resume/"days"/std::to_string(day)/"actions.txt")){
            const auto old=resume/"days"/std::to_string(day);
            const auto actions=read_actions(old/"actions.txt");
            const auto problem=load_problem_json(old/"problem.json");
            for(int h=0;h<24 && !sim.st.done;++h)step(&actions[h]);
            if(!endpoint_equal(sim.st.farms[0],problem,day+1))std::abort();
            const auto guard=guarded(actual,problem,actions);sequence.push_back(guard);
            const auto folder=directory/"days"/std::to_string(day);
            export_day(guard,name+"_d"+std::to_string(day),folder);save_problem_json(problem,folder/"problem.json");
            log<<day<<','<<problem.worker_count-source.problem.worker_count<<",1,1,0\n";log.flush();
            std::cout<<"day="<<day<<" exact_prefix_reused=1"<<std::endl;continue;
        }
        auto problem=source.problem;problem.start=actual.problem.start;
        // Unworked empty/weed squares belong to the actual trajectory.
        // A compiler cannot require a random weed to appear or disappear.
        for(int cell=0;cell<100;++cell){
            const auto& tile=sim.st.farms[0].tiles[cell/10][cell%10];
            auto& end=*problem.required_end_tiles[cell].exact_state;
            if(source_work(problem,cell).empty() && (tile.kind==T_EMPTY || tile.kind==T_WEED) &&
                (end.kind==ManagedTileKind::EMPTY || end.kind==ManagedTileKind::WEED || end.kind==ManagedTileKind::LOCKED))
                end=managed(tile,day+1);
        }
        auto markets=source.own;std::array<int,N_ITEMS> flow_delta{};std::array<int,N_CROPS> seed_delta{};
        std::array<int,N_ITEMS> new_inputs{},early_buys{};
        if(day==first){
            if(sim.st.farms[0].n_quadrants!=3)std::abort();
            if(!add_order(markets,0,2,M_BUY_LAND,0,1))return 3;
            for(int y=5;y<10;++y)for(int x=5;x<10;++x)
                problem.required_end_tiles[y*10+x].exact_state=ManagedTileState{};
        }

        std::vector<TileWork> replacements;
        for(const auto& r:rotations) {
            if(day<r.first)continue;
            const auto original=source_work(problem,r.cell);
            auto work=rotation_work(r,day,sim.st.farms[0].tiles[r.cell/10][r.cell%10],original);
            const auto expected=tile_endpoint(sim,r.cell,work);
            std::erase_if(problem.tile_work,[&](const auto& w){return w.tile==r.cell;});
            if(!work.empty())problem.tile_work.push_back({int16_t(r.cell),work});
            replacements.push_back({int16_t(r.cell),work});
            problem.required_end_tiles[r.cell].exact_state=managed(expected,day+1);
            const auto before=flow(original),after=flow(work);
            for(const auto& job:work){
                if(job.op==OP_FEED)++new_inputs[WHEAT];
                if(job.op==OP_FERTILIZE)++new_inputs[FERTILIZER];
            }
            for(const auto& job:original){
                if(job.op==OP_FEED)--new_inputs[WHEAT];
                if(job.op==OP_FERTILIZE)--new_inputs[FERTILIZER];
            }

            for(int i=0;i<N_ITEMS;++i)flow_delta[i]+=after[i]-before[i];
            for(const auto& job:original)if(job.op==OP_PLANT)++seed_delta[job.arg];
            for(const auto& job:work)if(job.op==OP_PLANT)--seed_delta[job.arg];
        }

        // Keeping a late sale's stock does not make it available for early
        // feed/fertilizer routes. Preserve old sales and buy the extra input
        // at an observed, funded hour; physical replay still must certify it.
        for(int item:{WHEAT,FERTILIZER}){
            const int surplus=std::max<int64_t>(0,actual.problem.start.shed[item]-source.problem.start.shed[item]);
            const int quantity=std::max(0,new_inputs[item]-surplus);
            if(!quantity)continue;
            auto funding_source=source;funding_source.start=sim;
            if(!insert_funded_order(funding_source,problem,markets,season.shops,M_BUY_PRODUCT,item,quantity)){
                std::ofstream(directory/"STATUS.json")<<"{\"status\":\"early_input_funding_failed\",\"day\":"<<day<<"}\n";
                return 3;
            }
            early_buys[item]=quantity;
        }
        // Cancel animal purchases displaced by this dated continuation before
        // searching a funded hour for the replacement. Already bought animals
        // are sunk stock: retain them for future placements instead of selling.
        std::array<int,N_ITEMS> canceled_animals{};
        for(int i=GOOSE;i<=SHEEP;++i){
            int surplus=actual.problem.start.shed[i]-source.problem.start.shed[i]+flow_delta[i];
            for(int h=23;h>=0 && surplus>0;--h)for(int s=markets[h].n_orders-1;s>=0 && surplus>0;--s){
                auto& order=markets[h].orders[s];
                if(order.op!=M_BUY_ANIMAL || order.item!=i)continue;
                const int n=std::min(surplus,order.n);order.n-=n;surplus-=n;canceled_animals[i]+=n;
                if(!order.n)order={};markets[h].finalize();
            }
        }
        // Preserve final useful stock. Replace lost feed by fewer wheat sales
        // or an explicit purchase; require surplus crops to be deposited/sold.
        for(int i=0;i<N_ITEMS;++i) {
            int delta=actual.problem.start.shed[i]-source.problem.start.shed[i]+flow_delta[i]+early_buys[i]-canceled_animals[i];
            delta+=season.end_farms[day].discarded[i]-source.start.st.farms[0].discarded[i];
            // Missing carried output invalidates the earliest inherited sales.
            // New same-day production may replace it economically, but cannot
            // be assumed present before a worker harvests and deposits it.
            if(i!=WHEAT && i!=FERTILIZER && !is_animal(i)){
                int missing=std::max<int64_t>(0,source.problem.start.shed[i]-actual.problem.start.shed[i]);
                for(int h=0;h<24 && missing>0;++h)for(int s=0;s<markets[h].n_orders && missing>0;++s){
                    auto& order=markets[h].orders[s];
                    if(order.op!=M_SELL || order.item!=i)continue;
                    const int remove=std::min(missing,order.n);
                    order.n-=remove;missing-=remove;delta+=remove;
                    if(!order.n)order={};markets[h].finalize();
                }
            }
            if(next_day_tomato_sales && i==TOMATO) {
                // Keep today's added tomatoes through the free day-end
                // deposit, and sell only the carried surplus next morning.
                // Keep only what fits after the other required end stock.
                // Excess output still needs a same-day deposit and sale.
                int occupied=0;for(int n:problem.end_shed)occupied+=n;
                const int extra=std::max(0,flow_delta[i]);
                const int carry=std::min(extra,std::max(0,config.shed_capacity-occupied));
                const int today=extra-carry;
                if(today && !add_order(markets,23,23,M_SELL,i,today))return 3;
                problem.end_shed[i]+=carry;delta-=carry+today;
            }
            if(delta<0)for(int h=23;h>=0 && delta<0;--h)for(int s=markets[h].n_orders-1;s>=0 && delta<0;--s) {
                auto& order=markets[h].orders[s];if(order.op!=M_SELL || order.item!=i)continue;
                const int remove=std::min(-delta,order.n);order.n-=remove;delta+=remove;
                if(!order.n)order={};markets[h].finalize();
            }
            if(delta<0 && (i!=WHEAT && i!=FERTILIZER && !is_animal(i))){
                const int reduce=std::min<int64_t>(-delta,problem.end_shed[i]);
                problem.end_shed[i]-=reduce;delta+=reduce;
                if(delta<0){std::cerr<<"unbuyable deficit day "<<day<<" item "<<i<<'\n';return 3;}
            }
            if(delta<0){
                bool added=false;
                if(is_animal(i)){
                    auto funding_source=source;funding_source.start=sim;
                    added=insert_funded_order(funding_source,problem,markets,season.shops,M_BUY_ANIMAL,i,-delta);
                }else added=add_order(markets,0,22,M_BUY_PRODUCT,i,-delta);
                if(!added){std::ofstream(directory/"STATUS.json")<<"{\"status\":\"funding_insertion_failed\",\"day\":"<<day<<"}\n";return 3;}
            }
            if(delta>0 && is_animal(i)){problem.end_shed[i]+=delta;delta=0;}
            if(delta>0) {
                const int hour=next_day_tomato_sales && i==TOMATO?0:23;
                const int last=next_day_tomato_sales && i==TOMATO?22:23;
                if(!add_order(markets,hour,last,M_SELL,i,delta))return 3;
            }
        }
        for(int i=0;i<N_CROPS;++i) {
            int delta=actual.problem.start.seeds[i]-source.problem.start.seeds[i]+seed_delta[i];
            if(cancel_unused_seeds && delta>0) {
                // Remove purchases covered by seeds the edited course no
                // longer consumes. Physical replay must still certify every
                // remaining planting's intraday seed availability.
                for(int h=23;h>=0 && delta>0;--h)for(int s=markets[h].n_orders-1;s>=0 && delta>0;--s) {
                    auto& order=markets[h].orders[s];
                    if(order.op!=M_BUY_SEED || order.item!=i)continue;
                    const int remove=std::min(delta,order.n);order.n-=remove;delta-=remove;
                    if(!order.n)order={};markets[h].finalize();
                }
            }
            problem.end_seeds[i]+=delta;
            if(problem.end_seeds[i]<0) {
                if(!add_order(markets,0,22,M_BUY_SEED,i,-problem.end_seeds[i]))return 3;
                problem.end_seeds[i]=0;
            }
        }
        bool kept=false;
        std::vector<int> query_order{0,1,2,3,4,5,6};
        std::array<std::string,7> deferred_inputs;
        std::array<double,7> deferred_probability{};
        for(size_t query_index=0;query_index<query_order.size() && !kept;++query_index) {
            const int hires=query_order[query_index];
            const bool retry=query_index>=7;
            auto trial=problem;auto orders=markets;
            for(int i=0;i<hires;++i)if(!add_order(orders,0,2,M_HIRE,0,1))return 3;
            if(day==29) {
                // Every sale must occur before the real last action, hour 22.
                for(int s=0;s<orders[23].n_orders;++s) {
                    const auto order=orders[23].orders[s];
                    if(order.op!=M_NONE && !add_order(orders,22,22,order.op,order.item,order.n))return 3;
                }
                orders[23].n_orders=0;orders[23].finalize();
            }
            market_contract(trial,orders,sim.st.farms[0].n_quadrants);
            day_scheduler::prepare_problem(trial);
            // The public API represents deadlines as event outcome bounds.
            // Prepare first: it rebuilds the derived outcome list.
            if(day==29){
                const std::pair<int,OutcomeMetric> metrics[]={
                    {OP_PLANT,OutcomeMetric::PLANTED},{OP_WATER,OutcomeMetric::CROP_WATERED},
                    {OP_HARVEST,OutcomeMetric::HARVESTED},{OP_FERTILIZE,OutcomeMetric::CROP_FERTILIZED},
                    {OP_DIG,OutcomeMetric::DUG},{OP_BUILD_PASTURE,OutcomeMetric::PASTURE_BUILT},
                    {OP_BUILD_COOP,OutcomeMetric::COOP_BUILT},{OP_PLACE,OutcomeMetric::ANIMAL_PLACED},
                    {OP_FEED,OutcomeMetric::FED},{OP_CARE,OutcomeMetric::CARED},
                    {OP_COLLECT_FERTILIZER,OutcomeMetric::FERTILIZER_COLLECTED}};
                for(auto [operation,metric]:metrics){
                    int count=0;for(const auto& work:trial.tile_work)for(const auto& job:work.actions)
                        if(job.op==operation)count+=operation==OP_HARVEST?job.output_quantity:1;
                    if(count)trial.required_outcomes.push_back({{metric,NO_SUBJECT,NO_TILE,22},count,std::numeric_limits<int64_t>::max()});
                }
            }
            const auto folder=directory/"days"/std::to_string(day);fs::create_directories(folder);
            save_problem_json(trial,folder/("problem_h"+std::to_string(hires)+".json"));
            save_actions(orders,folder/("orders_h"+std::to_string(hires)+".txt"));
            day_scheduler::Options budget;budget.seconds=seconds;budget.fallback_workers=1;
            day_scheduler::Result solved;
            if(hires==0 && !retry)solved.schedule=reuse_visits(source,trial,orders,replacements);
            const bool reused=bool(solved.schedule);
            if(!solved.schedule && !retry)solved=fixed_repair(source,trial,seconds);
            if(!solved.schedule) {
                double probability=deferred_probability[hires],prediction_seconds=0;
                if(retry) {
                    if(deferred_inputs[hires].empty() || deferred_inputs[hires]!=serialize_problem_json(trial))
                        throw std::runtime_error("deferred cold retry input changed");
                } else {
                    const auto start=std::chrono::steady_clock::now();
                    probability=warm_trial_probability(trial,day==29?23:24);
                    prediction_seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
                }
                const bool deferred=!retry && probability<0.10;
                guidance<<day<<','<<hires<<','<<retry<<','<<probability<<','<<deferred<<','<<prediction_seconds<<'\n';guidance.flush();
                if(deferred){
                    deferred_inputs[hires]=serialize_problem_json(trial);
                    deferred_probability[hires]=probability;
                    query_order.push_back(hires);continue;
                }
                solved=day_scheduler::solve(trial,budget);
            }
            bool equal=false;
            if(solved.schedule) {
                const auto certificate=replay_schedule(trial,*solved.schedule);
                if(!certificate.requirements_satisfied || !certificate.invariants_satisfied)std::abort();
                save_actions(*solved.schedule,folder/("raw_schedule_h"+std::to_string(hires)+".txt"));
                auto actions=*solved.schedule;
                for(int h=0;h<24;++h) {
                    actions[h].n_orders=orders[h].n_orders;
                    std::copy_n(orders[h].orders,orders[h].n_orders,actions[h].orders);actions[h].finalize();
                }
                save_actions(actions,folder/("executable_h"+std::to_string(hires)+".txt"));
                // The full live-opponent day must realize the exact requested
                // endpoint; a merely feasible physical schedule is insufficient.
                const auto saved=sim;auto saved_own=own;auto saved_rival=rival;
                for(int h=0;h<24 && !sim.st.done;++h)step(&actions[h]);
                auto endpoint=sim;
                if(day==29) {
                    endpoint.st.done=false;endpoint.cfg.episode_steps=744;
                    Action padding[2];for(int p=0;p<2;++p){padding[p].n_units=endpoint.st.farms[p].n_units;padding[p].finalize();}
                    endpoint.step(padding[0],padding[1]);
                }
                equal=endpoint_equal(endpoint.st.farms[0],trial,day+1);
                if(!equal) {
                    std::ofstream tiles(folder/("tiles_h"+std::to_string(hires)+".txt"));
                    for(int cell=0;cell<100;++cell){
                        const auto a=managed(endpoint.st.farms[0].tiles[cell/10][cell%10],day+1);
                        const auto b=*trial.required_end_tiles[cell].exact_state;
                        if(a!=b){
                            tiles<<cell<<" actual feed/care/dry/bank/yield "<<a.fed_today<<' '<<a.cared_today<<' '<<a.consecutive_dry_days<<' '<<a.pending_care_bonus<<' '<<a.stored_units
                                 <<" expected "<<b.fed_today<<' '<<b.cared_today<<' '<<b.consecutive_dry_days<<' '<<b.pending_care_bonus<<' '<<b.stored_units<<'\n';
                        }
                    }
                    auto trace=saved;auto trace_rival=saved_rival;
                    std::ofstream steps(folder/("trace_h"+std::to_string(hires)+".csv"));
                    steps<<"hour,unit,cell,operation,arg,quantity,wheat_before,shed_before,shed_after,fed_before,fed_after,care_before,care_after\n";
                    for(int h=0;h<24 && !trace.st.done;++h){
                        std::copy_n(season.shops.begin(),trace.st.n_shops,trace.st.shops);
                        Action other;trace_rival.act(agent::runtime::make_observation(trace,1),decision_budget(),other);
                        const auto before=trace;trace.step(actions[h],other);
                        for(int u=0;u<before.st.farms[0].n_units;++u){
                            const auto& f=before.st.farms[0];const int cell=f.pos_y[u]*10+f.pos_x[u];
                            const auto& a=f.tiles[cell/10][cell%10];const auto& b=trace.st.farms[0].tiles[cell/10][cell%10];
                            const auto& op=actions[h].units[u];
                            if(op.op==OP_FEED || op.op==OP_PICKUP || op.op==OP_PLACE || op.op==OP_DROP)
                                steps<<h<<','<<u<<','<<cell<<','<<+op.op<<','<<+op.arg<<','<<op.n<<','<<f.inv[u][WHEAT]<<','<<f.shed[WHEAT]<<','<<trace.st.farms[0].shed[WHEAT]<<','<<a.fed_today<<','<<b.fed_today<<','<<a.cared_today<<','<<b.cared_today<<'\n';
                        }
                    }
                    std::ofstream witness(folder/("endpoint_failure_h"+std::to_string(hires)+".json"));
                    witness<<"{\"day\":"<<day<<",\"expected_shed\":[";
                    for(int i=0;i<N_ITEMS;++i)witness<<(i?",":"")<<trial.end_shed[i];
                    witness<<"],\"actual_shed\":[";
                    for(int i=0;i<N_ITEMS;++i)witness<<(i?",":"")<<sim.st.farms[0].shed[i];
                    witness<<"],\"expected_seeds\":[";
                    for(int i=0;i<N_CROPS;++i)witness<<(i?",":"")<<trial.end_seeds[i];
                    witness<<"],\"actual_seeds\":[";
                    for(int i=0;i<N_CROPS;++i)witness<<(i?",":"")<<sim.st.farms[0].seeds[i];
                    witness<<"]}\n";
                }
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
    std::ofstream result(directory/"MATCHED_RESULT.json");
    result<<"{\"scope\":\"offline fixed-calendar matched scenario, not deployable policy evaluation\","
          <<"\"seed\":"<<seed<<",\"parent_cash\":"<<season.final_farms[0].money
          <<",\"parent_rival_cash\":"<<season.final_farms[1].money
          <<",\"cash\":"<<sim.st.farms[0].money<<",\"rival_cash\":"<<sim.st.farms[1].money;
    for(int p=0;p<2;++p){
        result<<",\"produced"<<p<<"\":[";
        for(int i=0;i<N_ITEMS;++i)result<<(i?",":"")<<sim.st.farms[p].produced[i];
        result<<"],\"parent_produced"<<p<<"\":[";
        for(int i=0;i<N_ITEMS;++i)result<<(i?",":"")<<season.final_farms[p].produced[i];
        result<<"]";
    }
    result<<"}\n";
    std::ofstream(directory/"STATUS.json")<<"{\"status\":\"compiled\",\"days\":"<<sequence.size()<<"}\n";
}
