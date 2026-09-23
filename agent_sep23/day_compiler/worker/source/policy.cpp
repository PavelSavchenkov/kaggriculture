#include "policy.hpp"
#include "verify.hpp"
#include "placement.hpp"
#include "day_jobs.hpp"
#include "agents/common/runtime/observation_builder.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#ifdef DAY_POLICY_DIAGNOSTICS
#include <cstdio>
#endif

namespace kag::agents::sep22_worker {
namespace detail {
bool valid_input(const DayInput& in) {
    if (in.hours < 1 || in.hours > 24 || in.start_hour<0 || in.start_hour>=in.hours ||
        in.worker_count<1 || in.worker_count>MAX_WORKERS || in.establish_count < 0 || in.establish_count > 100 ||
        in.land_hour < -1 || in.land_hour >= in.hours || (in.land_hour>=0 && in.land_hour<in.start_hour)) return false;
    int carried[N_ITEMS]{};
    for(int u=0;u<in.worker_count;++u) {
        const auto& worker=in.workers[u];
        if(worker.tile>=100 || worker.inventory_count>N_ITEMS) return false;
        bool seen[N_ITEMS]{};
        for(int k=0;k<worker.inventory_count;++k) {
            const int p=worker.inventory_keys[k];
            if(p>=N_ITEMS || seen[p] || worker.inventory[p]<=0) return false;
            seen[p]=true;
        }
        for(int p=0;p<N_ITEMS;++p) {
            if(worker.inventory[p]<0 || worker.inventory[p]>20000 || bool(worker.inventory[p])!=seen[p]) return false;
            carried[p]+=worker.inventory[p]; if(carried[p]>20000) return false;
        }
    }
    int quadrants = 0;
    for (int q = 0; q < 4; ++q) {
        int owned = 0;
        for (int c = 0; c < 100; ++c) if (quadrant_of(c%10,c/10,10) == q) owned += in.grid[c].kind != T_LOCKED;
        if (owned != 0 && owned != 25) return false;
        if (owned && q != quadrants++) return false;
    }
    if (!quadrants || (in.land_hour >= 0 && quadrants == 4)) return false;
    int need_f = 0, source_f = 0, seed_need[N_CROPS]{}, animal_need[3]{};
    for (int c = 0; c < 100; ++c) {
        const auto& t = in.grid[c]; const auto e = in.events[c];
        if (t.kind > T_PLANT || t.what >= N_ITEMS || t.planted_day > 0 ||
            (t.watered_today && (e&Water)) || (t.fed_today && (e&Feed)) || (t.cared_today && (e&Care)) || t.yield_units < 0) return false;
        const int allowed = t.has_animal ? Feed|Care|Harvest|CollectFertilizer :
            t.kind == T_PLANT ? Water|Fertilize|Harvest|Clear :
            (t.kind == T_WEED || t.kind == T_COOP || t.kind == T_PASTURE) ? Clear : 0;
        if (e & ~allowed) return false;
        if (t.kind == T_PLANT && !is_crop(t.what)) return false;
        if (t.has_animal && (!is_animal(t.what) || (t.kind != T_COOP && t.kind != T_PASTURE))) return false;
        if ((e & Harvest) && (t.yield_units <= 0 || (t.kind == T_PLANT && -t.planted_day < CROPS[t.what].first_yield_day))) return false;
        if ((e & CollectFertilizer) && !t.fertilizer_available) return false;
        need_f += bool(e & Fertilize); source_f += bool(e & CollectFertilizer);
    }
    for (int j = 0; j < in.establish_count; ++j) {
        const auto n = in.establish[j];
        if (is_crop(n.product)) { if (n.events & ~(Water|Fertilize)) return false; ++seed_need[n.product]; }
        else if (is_animal(n.product)) { if (n.events & ~(Feed|Care)) return false; ++animal_need[n.product-GOOSE]; }
        else return false;
        need_f += bool(n.events & Fertilize);
    }
    for (int it = 0; it < N_ITEMS; ++it) if (in.shed[it] < 0 || in.shed[it] > 20000) return false;
    int bought_seeds[N_CROPS]{}, bought_animals[3]{}, bought_fertilizer=0;
    int wheat = in.shed[WHEAT]+carried[WHEAT];
    for (int h = 0; h < 24; ++h) {
        int fixed_orders=in.land_hour==h;
        for(int c=0;c<N_CROPS;++c) {
            if(in.buy_seeds[h][c]<0)return false;
            bought_seeds[c]+=in.buy_seeds[h][c];fixed_orders+=in.buy_seeds[h][c]>0;
        }
        for(int a=0;a<3;++a) {
            if(in.buy_animals[h][a]<0)return false;
            bought_animals[a]+=in.buy_animals[h][a];fixed_orders+=in.buy_animals[h][a]>0;
        }
        if (in.buy_wheat[h] < 0 || in.buy_fertilizer[h] < 0) return false;
        wheat += in.buy_wheat[h];
        bought_fertilizer+=in.buy_fertilizer[h];
        fixed_orders+=in.buy_wheat[h]>0;fixed_orders+=in.buy_fertilizer[h]>0;
        if ((h >= in.hours || h<in.start_hour) && fixed_orders) return false;
        if (wheat > 20000 || fixed_orders>10) return false;
        for (int it = 0; it < N_PRODUCTS; ++it)
            if (in.returns[h][it] < (h ? in.returns[h-1][it] : 0) || (h<in.start_hour && in.returns[h][it])) return false;
    }
    for (int c = 0; c < N_CROPS; ++c)
        if (in.seeds[c] < 0 || in.seeds[c]+bought_seeds[c] > 20000 || seed_need[c] > in.seeds[c]+bought_seeds[c]) return false;
    for (int a = 0; a < 3; ++a)
        if (bought_animals[a]+in.shed[GOOSE+a]+carried[GOOSE+a] > 20000 || animal_need[a] > bought_animals[a]+in.shed[GOOSE+a]+carried[GOOSE+a]) return false;
    if(need_f>source_f+in.shed[FERTILIZER]+bought_fertilizer+carried[FERTILIZER])return false;
    return true;
}

Sim initial_state(const DayInput& in) {
    Config config; config.episode_steps = CALENDAR * 24 + in.hours + 1;
    config.starting_money = 1000000000;
    config.shed_capacity = 1000000; config.weed_chance = 0;
    Sim sim(config); sim.st.day = CALENDAR; sim.st.hour = in.start_hour; sim.st.step = CALENDAR * 24+in.start_hour;
    auto& f = sim.st.farms[0]; f.n_quadrants = 0;
    f.n_units=in.worker_count; f.hires_today=in.worker_count-1;
    for(int u=0;u<in.worker_count;++u) {
        const auto& worker=in.workers[u]; f.pos_y[u]=worker.tile/10; f.pos_x[u]=worker.tile%10;
        f.inv_clear(u);
        for(int k=0;k<worker.inventory_count;++k) {
            const int p=worker.inventory_keys[k]; f.inv_add(u,p,worker.inventory[p]);
        }
    }
    for (auto* mask : {f.empty_mask, f.plant_mask, f.animal_mask, f.decay_mask}) mask[0] = mask[1] = 0;
    f.next_decay_step = INT_MAX;
    for (int c = 0; c < 100; ++c) {
        auto t = in.grid[c];
        t.planted_day += CALENDAR; t.fertilized_until_day += CALENDAR;
        t.max_lifespan_step = t.max_lifespan_step == INT_MAX ? -1 : t.max_lifespan_step + CALENDAR * 24;
        f.tiles[c/10][c%10] = t;
        const uint64_t bit = uint64_t{1} << (c%64);
        if (t.kind == T_EMPTY) f.empty_mask[c/64] |= bit;
        if (t.kind == T_PLANT) f.plant_mask[c/64] |= bit;
        if (t.has_animal) f.animal_mask[c/64] |= bit;
        if (t.kind == T_PLANT && t.max_lifespan_step >= 0) {
            f.decay_mask[c/64] |= bit; f.next_decay_step = std::min(f.next_decay_step,t.max_lifespan_step);
        }
    }
    for (int c : {0,5,50,55}) f.n_quadrants += in.grid[c].kind != T_LOCKED;
    f.shed_total = 0;
    for (int it = 0; it < N_ITEMS; ++it) { f.shed[it] = in.shed[it]; f.shed_total += in.shed[it]; }
    for (int c = 0; c < N_CROPS; ++c) f.seeds[c] = in.seeds[c];
    return sim;
}

void advance(Sim& sim, const Action& action, int hour) {
    // The API stops before random night events and automatic cargo settlement.
    if (hour == 23) sim.st.hour = 22;
    sim.step(action, Action{});
    if (hour == 23) sim.st.hour = 24;
}
DayState export_state(const Sim& sim) {
    DayState out; const auto& f = sim.st.farms[0];
    for (int c = 0; c < 100; ++c) {
        auto t = f.tiles[c/10][c%10];
        t.planted_day -= CALENDAR; t.fertilized_until_day -= CALENDAR;
        t.max_lifespan_step = t.max_lifespan_step < 0 ? INT_MAX : t.max_lifespan_step - CALENDAR*24;
        out.grid[c] = t;
    }
    for (int it = 0; it < N_ITEMS; ++it) out.shed[it] = f.shed[it];
    for (int c = 0; c < N_CROPS; ++c) out.seeds[c] = f.seeds[c];
    out.worker_count = f.n_units;
    for (int u = 0; u < f.n_units; ++u) {
        out.workers[u].tile = f.pos_y[u]*10+f.pos_x[u];
        for (int it = 0; it < N_ITEMS; ++it) out.workers[u].inventory[it] = f.inv[u][it];
        out.workers[u].inventory_count=f.inv_nkeys[u];
        std::copy_n(f.inv_keys[u],f.inv_nkeys[u],out.workers[u].inventory_keys);
    }
    return out;
}
}

namespace {
bool can_supply_returns(const DayInput& input) {
    bool requested=false;
    for(int it=1;it<N_PRODUCTS;++it)requested |= input.returns[23][it]>0;
    if(!requested)return true;
    int available[24][N_PRODUCTS]{},collections=0,fertilizations=0;
    int carried_fertilizer=0;
    for(int u=0;u<input.worker_count;++u) {
        const auto& worker=input.workers[u];
        const int earliest=input.start_hour+shed_distance(worker.tile);
        carried_fertilizer+=worker.inventory[FERTILIZER];
        if(earliest<24) for(int p=1;p<N_PRODUCTS;++p) available[earliest][p]+=worker.inventory[p];
    }
    for(int cell=0;cell<100;++cell) {
        const auto e=input.events[cell];fertilizations+=bool(e&Fertilize);
        collections+=bool(e&CollectFertilizer);
        if(!(e&(Harvest|CollectFertilizer)))continue;
        Job job;job.tile=cell;
        auto add=[&](int op){job.steps[job.count++]={uint8_t(op),0,1};};
        if(e&Fertilize)add(OP_FERTILIZE);
        if(e&Water)add(OP_WATER);
        if(e&Harvest)add(OP_HARVEST);
        if(e&CollectFertilizer)add(OP_COLLECT_FERTILIZER);
        const int d=shed_distance(cell);
        // Give every producer an ideal worker and free prerequisite service.
        // This remains optimistic when several workers act on one tile.
        int approach=1+d;
        for(int u=0;u<input.worker_count;++u) approach=std::min(approach,distance(input.workers[u].tile,cell));
        const int earliest=input.start_hour+approach+d+1;
        for(int it=1;it<N_PRODUCTS;++it)if(earliest<24 && input.returns[23][it]>0)
            available[earliest][it]+=harvest_output(job,0,input.grid[cell],0,it);
    }
    for(int j=0;j<input.establish_count;++j)fertilizations+=bool(input.establish[j].events&Fertilize);
    int bought_fertilizer=0;
    for(int h=0;h<24;++h)bought_fertilizer+=input.buy_fertilizer[h];
    if(input.returns[23][FERTILIZER]>collections+carried_fertilizer ||
       fertilizations>collections+input.shed[FERTILIZER]+bought_fertilizer+carried_fertilizer)return false;
    for(int h=0;h<24;++h)for(int it=1;it<N_PRODUCTS;++it) {
        if(h)available[h][it]+=available[h-1][it];
        // This executor returns these products from declared field operations.
        // Wheat is exempt because picked-up feeding stock may also be returned.
        if(input.returns[h][it]>available[h][it])return false;
    }
    return true;
}


bool orders(const DayInput& in, DayPlan& plan, Action* actions, const SolveOptions& options) {
    auto emit = [&](int h, Order order) {
        if (actions[h].n_orders == 10) return false;
        actions[h].orders[actions[h].n_orders++] = order; return true;
    };
    for (int h = 0; h < 24; ++h) {
        for (int c = 0; c < N_CROPS; ++c) if (in.buy_seeds[h][c] && !emit(h,{M_BUY_SEED,uint8_t(c),in.buy_seeds[h][c]})) return false;
        for (int a = 0; a < 3; ++a) if (in.buy_animals[h][a] && !emit(h,{M_BUY_ANIMAL,uint8_t(GOOSE+a),in.buy_animals[h][a]})) return false;
        if (in.buy_wheat[h] && !emit(h,{M_BUY_PRODUCT,WHEAT,in.buy_wheat[h]})) return false;
        if (in.buy_fertilizer[h] && !emit(h,{M_BUY_PRODUCT,FERTILIZER,in.buy_fertilizer[h]})) return false;
    }
    if (in.land_hour >= 0 && !emit(in.land_hour,{M_BUY_LAND,0,1})) return false;
    int hired=in.worker_count-1;plan.first_wave=hired;
    for(int h=in.start_hour;h<in.hours && hired<plan.hires;++h)while(actions[h].n_orders<10-options.reserved_order_slots &&
        hired<plan.hires && h>=options.hire_not_before[hired+1]) {
        if(!emit(h,{M_HIRE,0,1}))return false;
        plan.hire_hour[++hired]=h;
        plan.first_wave+=h==in.start_hour;
    }
    if(hired!=plan.hires)return false;
    return true;
}
}

SolveResult Solver::solve(const DayInput& input, const SolveOptions& options) {
    const auto start = std::chrono::steady_clock::now();
    SolveResult best;
    const int existing_hires=input.worker_count-1;
    const bool known_effort=options.effort==SearchEffort::Fast || options.effort==SearchEffort::Full ||
        options.effort==SearchEffort::Compact || options.effort==SearchEffort::Balanced ||
        options.effort==SearchEffort::Classic || options.effort==SearchEffort::DayPolicy80p;
    const bool invalid_80p=options.effort==SearchEffort::DayPolicy80p &&
        (options.max_hires!=11 || options.minimize_hires);
    if (int(options.placement)<0 || int(options.placement)>int(PlacementStyle::Staged) || !detail::valid_input(input) || options.max_hires < existing_hires || options.max_hires >= MAX_WORKERS || options.variants < 1 || options.variants > 16 || options.minimize_variants<1 || options.minimize_variants>options.variants || options.route_rounds<0 || options.route_rounds>4 || options.animal_reserve<0 || options.animal_reserve>25 || !known_effort || invalid_80p || options.reserved_order_slots<0 || options.reserved_order_slots>9) {
        best.status = SolveStatus::InvalidInput;
        best.microseconds=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        return best;
    }
    if(!can_supply_returns(input)) {
        best.microseconds=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        return best;
    }
    const auto initial = detail::initial_state(input);
    const auto observation = agent::runtime::make_observation(initial,0);
    const auto init = agent::runtime::make_agent_init(initial,0);
    const int variants[] = {2064|4096|512,2072|32768|4096|512,2064|4096,2076|4096|512,2048|4096|512,2072|128|4096|512,2072|64|4096|512,2072|128|4096};
    int attempts = 0;
    auto stopped = [&] { return uint64_t(attempts) >= options.max_attempts || options.budget.soft_expired() || options.budget.hard_expired(); };
    static_assert(N_PRODUCTS <= 32);
    uint32_t deadline_products[24]{};
    for (int h = 0; h < 24; ++h) for (int it = 0; it < N_PRODUCTS; ++it)
        if (input.returns[h][it] > (h ? input.returns[h-1][it] : 0)) deadline_products[h] |= uint32_t{1} << it;
    struct WinningSeed { int variant = 0, family = 0; bool legacy = false, split = false; } winning;
    auto try_hires = [&](int hires,int width,bool legacy,bool split,int family,int offset=0) {
        if (hires<existing_hires || stopped()) return false;
        std::array<int8_t,MAX_WORKERS> spawns; spawns.fill(-1);
        std::array<int8_t,MAX_WORKERS> resume_spawns;
        int retries=0;
        for (int trial = width*family; trial < width*(family+1); ++trial) {
            if (stopped()) return false;
            const int variant=trial%width+offset;
            const bool repair=family>0;
            const int base=variant%8;
            if(!retries && (variant==0 || variant==8)) spawns.fill(-1);
            ++attempts; SolveResult result; result.hires = hires;
            auto plan = compile_day_jobs(input,hires,variant,split);
            for (int j=0;j<plan.count;++j) if (plan.jobs[j].key>=0 && plan.jobs[j].key<100)
                for (int k=0;k<plan.jobs[j].count;++k) if (plan.jobs[j].steps[k].op==OP_HARVEST)
                    plan.jobs[j].deadline=std::min(plan.jobs[j].deadline,options.harvest_deadline[plan.jobs[j].key]);
            plan.delivery_batch_cap=family==6?8:0;
            plan.spawn_tile = spawns;
            Action fixed[24]{};
            if (!orders(input,plan,fixed,options)) continue;
            place_day(observation,plan,options.animal_reserve,!legacy,options.placement);
            bool placed = true;
            for (int j=0;j<plan.count;++j) placed &= plan.jobs[j].tile >= 0;
            if (!placed) continue;
            executor_.reset(init); executor_.set_plan(plan);
            DayReturns returns; std::copy_n(&input.returns[0][0],24*N_PRODUCTS,&returns.target[0][0]);
            executor_.set_returns(returns); executor_.set_fixed_orders(fixed);
            Options tuning; tuning.route_variant = variants[base] | FIELD_FERTILIZER | (variant>=8 ? FINISH_TILE_BEFORE_RETURN : 0);
            tuning.return_source_seed=family==4?1:family==5?2:family==6?base%2:0;
            tuning.reachable_sources=!legacy;
            tuning.deadline_deposits=!legacy;
            if(!legacy)tuning.route_variant|=EXACT_BIRTH_TIMING;
            if(split)tuning.route_variant|=SHARED_DROP_SCORE;
            tuning.route_variant |= REORDER_ROUTE_JOBS | BEAM_ASSIGNMENT_SEED;
            const int batches[]={1,1,1,2,3,1,2,1};
            tuning.route_variant |= (batches[base]-1)<<27;
            if(repair) tuning.route_variant |= REORDER_ROUTE_JOBS|REASSIGN_ROUTE_JOBS;
            if(base==1 || base==3 || base==5) tuning.route_variant |= ANIMAL_SERVICE_SEED;
            if(family>=2) tuning.route_variant |= EXCHANGE_ROUTE_TAILS;
            if(family>=3) tuning.route_variant |= SEGMENTED_PICKUPS;
            if(family==7)tuning.route_variant |= REGRET_ASSIGNMENT;
            tuning.route_rounds = options.route_rounds + (base>=6); executor_.set_options(tuning);
            executor_.prepare_routes(observation,options.budget);
            bool preposition=false;
            for(int j=0;j<executor_.plan().count;++j) {
                const auto& job=executor_.plan().jobs[j];
                preposition |= (job.new_site && job.predecessor>=0) || job.service_predecessor>=0;
            }
            const bool relocate=hires>=8 && input.establish_count>0;
            const bool urgent=hires>=8 && std::any_of(input.returns[23],input.returns[23]+N_PRODUCTS,[](int n){return n>0;});
            if(preposition || relocate || urgent)prepared_=executor_;
            int missing=0,old_missing=0;
            auto execute=[&](bool reject_late = false) {
                auto sim = initial;
                bool deadlines_met=true;
                int receipts[N_PRODUCTS]{};
                for(int h=0;h<input.start_hour;++h) {
                    result.schedule[h].clear(); result.schedule[h].n_units=input.worker_count;
                    std::fill_n(result.schedule[h].units,input.worker_count,UnitAction{}); result.schedule[h].finalize();
                }
                for (int h = input.start_hour; h < input.hours; ++h) {
                    const int known = sim.st.farms[0].n_units;
                    executor_.act(agent::runtime::make_observation(sim,0),{},result.schedule[h]);
#ifdef DAY_POLICY_DIAGNOSTICS
                    if(hires==options.max_hires && variant==3 && h==0) {
                        const auto& routes=executor_.route_diagnostics();
                        for(int u=0;u<=hires;++u){std::fprintf(stderr,"ROUTE u%d start%d length%d:",u,routes.start[u],routes.predicted_length[u]);for(int k=0;k<routes.count[u];++k)std::fprintf(stderr," %d",routes.jobs[u][k]);std::fprintf(stderr,"\n");}
                    }
#endif
                    for(int u=0;u<known;++u) {
                        const auto a=result.schedule[h].units[u];
                        if(a.op==OP_HARVEST) {
                            const auto& farm=sim.st.farms[0];
                            deadlines_met &= h<=options.harvest_deadline[farm.pos_y[u]*10+farm.pos_x[u]];
                        }
                        if(a.op==OP_PLACE && a.arg<N_PRODUCTS) receipts[a.arg]+=std::min<int>(a.n,sim.st.farms[0].inv[u][a.arg]);
                        if(a.op==OP_DROP) for(int it=0;it<N_PRODUCTS;++it) receipts[it]+=sim.st.farms[0].inv[u][it];
                    }
                    std::copy_n(receipts,N_PRODUCTS,result.receipts[h]);
                    detail::advance(sim,result.schedule[h],h);
                    const auto& f = sim.st.farms[0];
                    for(int u=known;u<f.n_units;++u) spawns[u] = f.pos_y[u]*10+f.pos_x[u];
                    // Only retries may stop early: the primary run's final
                    // unfinished-work count decides which retries are allowed.
                    if (reject_late) for (auto due = deadline_products[h]; due; due &= due - 1) {
                        const int it = std::countr_zero(due);
                        if (receipts[it] < input.returns[h][it]) return false;
                    }
                }
                for (int h=input.hours;h<24;++h) {
                    result.schedule[h].clear(); result.schedule[h].n_units=sim.st.farms[0].n_units;
                    std::fill_n(result.schedule[h].units,result.schedule[h].n_units,UnitAction{});
                    result.schedule[h].finalize(); std::copy_n(receipts,N_PRODUCTS,result.receipts[h]);
                }
                missing=old_missing=0;
                for (int j = 0; j < executor_.plan().count; ++j) {
                    const auto& job=executor_.plan().jobs[j];
                    const int left=job.count-executor_.completed_steps(j);
                    missing+=left;
                    if(!job.new_site)old_missing+=left;
                }
#ifdef DAY_POLICY_DIAGNOSTICS
                if(hires==options.max_hires) {
                    std::fprintf(stderr,"hires=%d family=%d variant=%d",hires,family,variant);
                    for(int j=0;j<executor_.plan().count;++j)if(executor_.completed_steps(j)!=executor_.plan().jobs[j].count){const auto& x=executor_.plan().jobs[j];std::fprintf(stderr," j%d(t%d %d/%d op%d due%d)",j,x.tile,executor_.completed_steps(j),x.count,x.steps[executor_.completed_steps(j)].op,x.deadline);}
                    std::fprintf(stderr,"\n");
                    if(variant==3){auto trace=initial;for(int h=input.start_hour;h<input.hours;++h){std::fprintf(stderr,"H%d",h);for(int u=0;u<result.schedule[h].n_units;++u){auto a=result.schedule[h].units[u];const auto& f=trace.st.farms[0];std::fprintf(stderr," u%d@%d:%d,%d,%d",u,f.pos_y[u]*10+f.pos_x[u],a.op,a.arg,a.n);}std::fprintf(stderr,"\n");detail::advance(trace,result.schedule[h],h);}
                    for(int j=0;j<executor_.plan().count;++j){const auto& x=executor_.plan().jobs[j];std::fprintf(stderr,"J%d t%d pred%d due%d:",j,x.tile,x.predecessor,x.deadline);for(int k=0;k<x.count;++k)std::fprintf(stderr," %d,%d,%d",x.steps[k].op,x.steps[k].arg,x.steps[k].n);std::fprintf(stderr,"\n");}
                    for(int u=0;u<sim.st.farms[0].n_units;++u){std::fprintf(stderr,"CARGO u%d pos%d:",u,sim.st.farms[0].pos_y[u]*10+sim.st.farms[0].pos_x[u]);for(int it=0;it<N_ITEMS;++it)if(sim.st.farms[0].inv[u][it])std::fprintf(stderr," %d=%d",it,sim.st.farms[0].inv[u][it]);std::fprintf(stderr,"\n");}}
                }
#endif
                if (missing || !deadlines_met) return false;
                result.state = detail::export_state(sim); result.status = SolveStatus::Success;
                for(int it=0;it<N_PRODUCTS;++it) result.production[it]=sim.st.farms[0].produced[it];
                const auto checked = verify(input,result);
                if (!checked.valid) {
#ifdef DAY_POLICY_DIAGNOSTICS
                    std::fprintf(stderr,"check failed h%d %s\n",checked.failed_hour,checked.reason);
#endif
                    return false;
                }
                best = result;
                winning = {variant, family, legacy, split};
                return true;
            };
            if(execute())return true;
            bool changed_spawn=false;
            for(int u=1;u<=hires;++u)changed_spawn |= plan.spawn_tile[u]>=0 && plan.spawn_tile[u]!=spawns[u];
            const bool retry=!retries && changed_spawn && missing<=8;
            auto next_trial=[&]{if(retry){resume_spawns=spawns;--trial;retries=1;}else {if(retries)spawns=resume_spawns;retries=0;}};

            const int primary_missing=old_missing;
            for(int mode=1;primary_missing<=8 && mode<16;++mode) {
                if (stopped()) return false;
                if((mode&1) && !preposition)continue;
                if((mode&2) && !relocate)continue;
                if((mode&4) && !urgent)continue;
                if((mode&8) && !(mode&4))continue;
                const auto primary_spawns=spawns;
                executor_=prepared_;
                auto execution=tuning;
                execution.preposition=mode&1;execution.relocate_idle=mode&2;
                if(mode&4)execution.route_variant|=URGENT_RETURNS;
                execution.match_routes=mode&8;
                executor_.set_options(execution);
                ++attempts;
                if(execute(true))return true;
                spawns=primary_spawns;
            }
            next_trial();
        }
        return false;
    };
    bool split_returns=false,alternative_timing=input.land_hour>=0 && input.establish_count>0;
    for(int it=0;it<N_PRODUCTS;++it)alternative_timing |= input.returns[23][it]>0;
    for(int c=0;c<100;++c) {
        const auto& t=input.grid[c];
        split_returns |= t.kind==T_PLANT && !CROPS[t.what].ongoing &&
            (input.events[c]&(Water|Harvest))==(Water|Harvest) && input.returns[23][t.what]>0;
        if(t.kind==T_PLANT && t.yield_units>0 && t.max_lifespan_step!=INT_MAX && input.events[c])
            alternative_timing |= int64_t(t.max_lifespan_step)+2*(t.yield_units-1)<24;
    }
    int work = input.establish_count * 2;
    for (int c = 0; c < 100; ++c) work += std::popcount(input.events[c]);
    auto search_hires=[&](int hires,int width) {
        if(options.effort==SearchEffort::DayPolicy80p) {
            struct Group { uint8_t family, offset, flags, minimum_work; };
            constexpr Group groups[] = {
                {5,0,0,0}, {6,8,2,156}, {6,0,0,114},
                {4,8,0,112}, {6,0,1,138}, {3,0,1,119}
            };
            for(const auto group:groups) {
                if(work<group.minimum_work)continue;
                if(try_hires(hires,8,group.flags&1,group.flags&2,group.family,group.offset))return true;
            }
            return false;
        }
        if(options.effort==SearchEffort::Balanced) {
            const bool legacy=!alternative_timing;
            const int families=width==1?2:8;
            // Finish the compact search before paying for extra timing,
            // cooperative service, or finish-before-return alternatives.
            for(int family=0;family<families;++family)
                if(try_hires(hires,width,legacy,false,family))return true;
            if(alternative_timing)for(int family=0;family<2;++family)
                if(try_hires(hires,width,!legacy,false,family))return true;
            if(split_returns && hires>=8)for(int family=0;family<2;++family)
                if(try_hires(hires,width,false,true,family))return true;
            if(hires>=11 && width<=8)for(int family=0;family<2;++family) {
                if(try_hires(hires,width,legacy,false,family,8))return true;
                if(alternative_timing && try_hires(hires,width,!legacy,false,family,8))return true;
                if(split_returns && try_hires(hires,width,false,true,family,8))return true;
            }
            return false;
        }
        const bool classic=options.effort==SearchEffort::Classic;
        const bool compact=options.effort==SearchEffort::Compact || classic;
        const bool legacy_first=classic || (!compact && hires<8) || !alternative_timing;
        const bool timing_modes=alternative_timing && !compact;
        const int families=width==1 || options.effort==SearchEffort::Fast ? 2 : 8;
        const bool finish=hires>=11 && width<=8 && options.effort==SearchEffort::Full;
        // Each family starts with fresh spawn estimates. Try its inexpensive
        // alternatives before advancing to heavier route repair.
        for(int family=0;family<families;++family) {
            if(try_hires(hires,width,legacy_first,false,family))return true;
            if(timing_modes && try_hires(hires,width,!legacy_first,false,family))return true;
            if(finish && ((hires==11 && options.effort==SearchEffort::Full) || family<2)) {
                if(try_hires(hires,width,legacy_first,false,family,8))return true;
                if(timing_modes && try_hires(hires,width,!legacy_first,false,family,8))return true;
            }
        }
        if(!compact && split_returns && hires>=8)for(int family=0;family<families;++family) {
            if(try_hires(hires,width,false,true,family))return true;
            if(finish && ((hires==11 && options.effort==SearchEffort::Full) || family<2) && try_hires(hires,width,false,true,family,8))return true;
        }
        return false;
    };
    int first = options.minimize_hires?std::min(11,options.max_hires):options.max_hires;
    if(options.minimize_hires) first = std::min(first,std::max(0,(work+7)/8-1));
    if(options.first_hires>=0) first=std::min(options.first_hires,options.max_hires);
    if (work == 0 && options.effort!=SearchEffort::DayPolicy80p) first = 0;
    first=std::max(first,existing_hires);
    for (int hires = first; hires <= options.max_hires; ++hires)
        if (search_hires(hires,options.variants)) break;
    if (best.status == SolveStatus::Success && options.minimize_hires && best.hires==first) {
        for (int hires = best.hires - 1; hires >= existing_hires; --hires) {
            bool improved = false;
            // Reuse a successful construction before falling back to the narrow
            // workforce search. Two cheap extra seeds cover common sparse days.
            if (options.effort == SearchEffort::Balanced && options.minimize_variants == 1 &&
                (winning.variant != 0 || winning.family >= 2 || (winning.split && hires < 8))) {
                const auto seed = winning;
                improved = try_hires(hires, 1, seed.legacy, seed.split, seed.family, seed.variant);
            }
            if (!improved) improved = search_hires(hires,options.minimize_variants);
            if (!improved && options.effort == SearchEffort::Balanced && options.minimize_variants == 1)
                for (int variant = 1; variant <= 2 && !improved; ++variant)
                    improved = try_hires(hires, 1, !alternative_timing, false, 0, variant);
            if (!improved) break;
        }
    }
    if(best.status==SolveStatus::Success && options.opportunistic_hire_reduction && !options.minimize_hires) {
        const int floor=std::max(existing_hires,attempts==1 ? (work<=30?3:work<=40?4:work<=60?5:13) : 13);
        for(int hires=best.hires-1;hires>=floor;--hires) {
            const auto seed=winning;
            if(!try_hires(hires,1,seed.legacy,seed.split,seed.family,seed.variant))break;
        }
    }
    // With a complete 13-hire incumbent, spend the remaining finish variants
    // on reducing it to 12. Failed plans do not pay this minimization cost.
    if(best.status==SolveStatus::Success && best.hires==13 && existing_hires<=12 && options.minimize_hires &&
       options.effort==SearchEffort::Full && options.variants<=8) {
        bool reduced=false;
        for(int family=2;family<8 && !reduced;++family) {
            reduced=try_hires(12,options.variants,false,false,family,8);
            if(!reduced && alternative_timing)reduced=try_hires(12,options.variants,true,false,family,8);
        }
        if(split_returns)for(int family=2;family<8 && !reduced;++family)
            reduced=try_hires(12,options.variants,false,true,family,8);
    }
    best.attempts = attempts;
    best.microseconds = std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    return best;
}

SolveOptions day_policy_80p() {
    SolveOptions options;
    options.effort=SearchEffort::DayPolicy80p;
    options.max_hires=11;
    options.variants=8;
    options.minimize_hires=false;
    return options;
}

SolveOptions unrestricted_day_policy() {
    SolveOptions options;
    options.effort=SearchEffort::Balanced;
    options.max_hires=13;
    options.variants=4;
    options.minimize_hires=false;
    options.opportunistic_hire_reduction=true;
    return options;
}
}
