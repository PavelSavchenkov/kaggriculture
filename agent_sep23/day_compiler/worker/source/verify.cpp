#include "verify.hpp"
#include <algorithm>
#include <tuple>
#include <chrono>

namespace kag::agents::sep22_worker {
namespace {
bool same_tile(const Tile& a, const Tile& b) {
    auto fields = [](const Tile& t) {
        return std::tuple(t.kind,t.what,t.has_animal,t.watered_today,t.fed_today,t.cared_today,
            t.fertilizer_available,t.consecutive_dry,t.yield_units,t.pending_care_bonus,
            t.planted_day,t.max_lifespan_step,t.fertilized_until_day);
    };
    return fields(a) == fields(b);
}
int event(int op) {
    switch (op) {
        case OP_WATER: return Water;
        case OP_FERTILIZE: return Fertilize;
        case OP_HARVEST: return Harvest;
        case OP_DIG: return Clear;
        case OP_FEED: return Feed;
        case OP_CARE: return Care;
        case OP_COLLECT_FERTILIZER: return CollectFertilizer;
        default: return 0;
    }
}
}
Verification verify(const DayInput& in, const SolveResult& result) {
    Verification check;
    auto fail = [&](int hour, const char* reason) {
        check.failed_hour=hour; check.reason=reason; return check;
    };
    if (!detail::valid_input(in) || result.status != SolveStatus::Success) return fail(-1,"invalid input or unsuccessful result");
    auto sim = detail::initial_state(in);
    int generation[100], seen[200]{}, product[200]{}, created=0, built[100]{}, prepared[100]{};
    int deposited[N_PRODUCTS]{}, hires=in.worker_count-1;
    for (int c=0;c<100;++c) generation[c]=c;
    for(int h=0;h<in.start_hour;++h) {
        const auto& action=result.schedule[h];
        if(action.n_orders) return fail(h,"orders before suffix start");
        for(int u=0;u<action.n_units;++u) if(action.units[u].op!=OP_PASS) return fail(h,"work before suffix start");
        for(int p=0;p<N_PRODUCTS;++p) if(result.receipts[h][p]) return fail(h,"receipts before suffix start");
    }
    for (int h=in.start_hour;h<in.hours;++h) {
        const auto& action=result.schedule[h]; const auto& farm=sim.st.farms[0];
        if (action.n_units!=farm.n_units || action.n_orders<0 || action.n_orders>10 || !action.metadata_ready)
            return fail(h,"action dimensions or metadata");
        auto metadata=action; metadata.finalize();
        if (metadata.plant_mask!=action.plant_mask || !std::equal(metadata.plant_demand,metadata.plant_demand+N_CROPS,action.plant_demand))
            return fail(h,"plant metadata");
#ifdef DAY_POLICY_FULL_HASH_CHECK
        constexpr bool localized_hash=false;
#else
        constexpr bool localized_hash=true;
#endif
        const auto outcome=sim.diagnose_solo_action(0,action,localized_hash);
        if(outcome.requested_unit_actions!=outcome.successful_unit_actions || outcome.requested_order_units!=outcome.successful_order_units)
            return fail(h,"ineffective action or order");
        Tile tiles[100]; for(int c=0;c<100;++c) tiles[c]=farm.tiles[c/10][c%10];
        for(int u=0;u<action.n_units;++u) {
            const auto a=action.units[u];
            const int c=farm.pos_y[u]*10+farm.pos_x[u], g=generation[c]; auto& t=tiles[c];
            if(a.op==OP_DROP) {
                for(int it=0;it<N_ITEMS;++it) if(farm.inv[u][it]) {
                    if(it>=N_PRODUCTS) return fail(h,"unrequested item deposit");
                    deposited[it]+=farm.inv[u][it];
                }
                continue;
            }
            const bool animal_place=a.op==OP_PLACE && is_animal(a.arg) && !t.has_animal &&
                t.kind==(ANIMALS[a.arg-GOOSE].structure==ST_COOP ? T_COOP : T_PASTURE);
            if(a.op==OP_PLANT || animal_place) {
                if(created==in.establish_count || g>=100) return fail(h,"extra establishment");
                if((in.events[c]&~seen[c])!=0) return fail(h,"replaced unfinished existing job");
                generation[c]=100+created; product[100+created++]=a.arg;
                t.kind=animal_place ? t.kind : T_PLANT; t.has_animal=animal_place; t.what=a.arg;
                continue;
            }
            if(a.op==OP_PLACE) {
                if(a.arg>=N_PRODUCTS) return fail(h,"unrequested item deposit");
                deposited[a.arg]+=std::min<int>(a.n,farm.inv[u][a.arg]);
                continue;
            }
            if(a.op==OP_BUILD_COOP || a.op==OP_BUILD_PASTURE) {
                if(built[c]++) return fail(h,"extra housing");
                t.kind=a.op==OP_BUILD_COOP ? T_COOP : T_PASTURE; continue;
            }
            const int e=event(a.op); if(!e) continue;
            if(e==Clear && g<100 && !(in.events[g]&Clear)) {
                if((t.kind!=T_WEED && t.kind!=T_COOP && t.kind!=T_PASTURE) || t.has_animal || prepared[c]++)
                    return fail(h,"unrequested clearing");
                t=Tile{}; continue;
            }
            if(seen[g]&e) return fail(h,"duplicate event");
            if(g<100 && !(in.events[g]&e)) return fail(h,"extra existing event");
            if(g>=100 && (e & ~(t.has_animal ? Feed|Care : Water|Fertilize))) return fail(h,"extra new-product event");
            if(e==Water && t.kind==T_PLANT && !CROPS[t.what].ongoing && t.fertilized_until_day<detail::CALENDAR) {
                const auto& cd=CROPS[t.what]; const int age=detail::CALENDAR-t.planted_day;
                const bool affects=age>=(cd.max_yield_day+1)/2 && age<=cd.max_yield_day && t.yield_units+1<cd.max_yield;
                if(g<100 && affects && (in.events[g]&Fertilize) && !(seen[g]&Fertilize)) return fail(h,"fertilizer must precede watering");
            }
            seen[g]|=e;
            if(e==Fertilize) t.fertilized_until_day=detail::CALENDAR+2;
            if(e==Clear || (e==Harvest && t.kind==T_PLANT && !CROPS[t.what].ongoing)) {
                if(g<100 && (in.events[g]&Clear)) seen[g]|=Clear;
                t=Tile{};
            }
        }
        int seeds[N_CROPS]{}, animals[3]{}, wheat=0, fertilizer=0, land=0;
        for(int k=0;k<action.n_orders;++k) {
            const auto o=action.orders[k];
            switch(o.op) {
                case M_HIRE: if(++hires>=MAX_WORKERS) return fail(h,"engine worker limit"); break;
                case M_BUY_SEED: if(!is_crop(o.item)) return fail(h,"seed purchase item"); seeds[o.item]+=o.n; break;
                case M_BUY_ANIMAL: if(!is_animal(o.item)) return fail(h,"animal purchase item"); animals[o.item-GOOSE]+=o.n; break;
                case M_BUY_PRODUCT:
                    if(o.item==WHEAT)wheat+=o.n;
                    else if(o.item==FERTILIZER)fertilizer+=o.n;
                    else return fail(h,"forbidden purchase");
                    break;
                case M_BUY_LAND: ++land; break;
                default: return fail(h,"unrequested market order");
            }
        }
        for(int c=0;c<N_CROPS;++c) if(seeds[c]!=in.buy_seeds[h][c]) return fail(h,"seed purchase mismatch");
        for(int a=0;a<3;++a) if(animals[a]!=in.buy_animals[h][a]) return fail(h,"animal purchase mismatch");
        if(wheat!=in.buy_wheat[h] || fertilizer!=in.buy_fertilizer[h] || land!=(in.land_hour==h))
            return fail(h,"product or land purchase mismatch");
        for(int it=0;it<N_PRODUCTS;++it) {
            check.receipts[h][it]=deposited[it];
            if(result.receipts[h][it]!=deposited[it]) return fail(h,"receipt output mismatch");
            if(deposited[it]<in.returns[h][it]) return fail(h,"return deadline");
        }
        detail::advance(sim,action,h);
    }
    for(int h=in.hours;h<24;++h) {
        const auto& action=result.schedule[h];
        if(action.n_orders) return fail(h,"orders outside horizon");
        for(int u=0;u<action.n_units;++u) if(action.units[u].op!=OP_PASS) return fail(h,"work outside horizon");
        for(int it=0;it<N_PRODUCTS;++it) if(in.returns[h][it]>deposited[it]) return fail(h,"return after horizon");
    }
    if(hires!=result.hires || created!=in.establish_count) return fail(24,"hire or establishment count");
    for(int c=0;c<100;++c) {
        if(seen[c]!=in.events[c]) return fail(24,"missing existing event");
        if((built[c] || prepared[c]) && generation[c]<100) return fail(24,"unused site preparation");
    }
    bool matched[100]{};
    for(int g=100;g<100+created;++g) {
        int j=0;
        for(;j<in.establish_count;++j) if(!matched[j] && in.establish[j].product==product[g] && in.establish[j].events==seen[g]) break;
        if(j==in.establish_count) return fail(24,"new-product events mismatch");
        matched[j]=true;
    }
    const auto expected=detail::export_state(sim);
    for(int it=0;it<N_PRODUCTS;++it)if(result.production[it]!=sim.st.farms[0].produced[it])return fail(24,"production mismatch");
    if(expected.worker_count!=result.state.worker_count) return fail(24,"worker state mismatch");
    for(int c=0;c<100;++c) if(!same_tile(expected.grid[c],result.state.grid[c])) return fail(24,"grid state mismatch");
    for(int it=0;it<N_ITEMS;++it) if(expected.shed[it]!=result.state.shed[it]) return fail(24,"shed state mismatch");
    for(int c=0;c<N_CROPS;++c) if(expected.seeds[c]!=result.state.seeds[c]) return fail(24,"seed state mismatch");
    for(int u=0;u<expected.worker_count;++u) {
        if(expected.workers[u].tile!=result.state.workers[u].tile) return fail(24,"worker position mismatch");
        for(int it=0;it<N_ITEMS;++it) if(expected.workers[u].inventory[it]!=result.state.workers[u].inventory[it]) return fail(24,"cargo mismatch");
        const auto& worker=expected.workers[u]; const auto& reported=result.state.workers[u];
        if(worker.inventory_count!=reported.inventory_count ||
           !std::equal(worker.inventory_keys,worker.inventory_keys+worker.inventory_count,reported.inventory_keys))
            return fail(24,"cargo insertion order mismatch");
    }
    check.valid=true; check.reason="complete"; return check;
}

Verification replay_schedule(const DayInput& in,const Action* schedule,SolveResult& result,const SolveOptions& options) {
    const auto began=std::chrono::steady_clock::now(); result=SolveResult{};
    auto fail=[&](int hour,const char* reason) {
        result.microseconds=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-began).count();
        Verification check; check.failed_hour=hour; check.reason=reason; return check;
    };
    if(!detail::valid_input(in) || !schedule || options.max_hires<in.worker_count-1)
        return fail(-1,"invalid input or missing program");
    auto sim=detail::initial_state(in); int deposited[N_PRODUCTS]{};
    for(int h=0;h<24;++h) {
        auto& action=result.schedule[h]; action.clear(); action.n_units=sim.st.farms[0].n_units;
        std::fill_n(action.units,action.n_units,UnitAction{}); action.finalize();
        if(h>=in.start_hour && h<in.hours) {
            if(options.budget.hard_expired()) return fail(h,"program check deadline");
            action=schedule[h]; const auto& farm=sim.st.farms[0];
            if(action.n_units!=farm.n_units || action.n_orders<0 || action.n_orders>10-options.reserved_order_slots)
                return fail(h,"program dimensions or reserved market slots");
            int hired=farm.n_units-1;
            for(int j=0;j<action.n_orders;++j) if(action.orders[j].op==M_HIRE) {
                if(++hired>options.max_hires || hired>=MAX_WORKERS || h<options.hire_not_before[hired])
                    return fail(h,"program hire constraint");
            }
            for(int u=0;u<action.n_units;++u) {
                const auto a=action.units[u];
                if(a.op==OP_HARVEST && h>options.harvest_deadline[farm.pos_y[u]*10+farm.pos_x[u]])
                    return fail(h,"program harvest deadline");
                if(a.op==OP_PLACE && a.arg<N_PRODUCTS) deposited[a.arg]+=std::min<int>(a.n,farm.inv[u][a.arg]);
                if(a.op==OP_DROP) for(int p=0;p<N_PRODUCTS;++p) deposited[p]+=farm.inv[u][p];
            }
            detail::advance(sim,action,h);
        }
        std::copy_n(deposited,N_PRODUCTS,result.receipts[h]);
    }
    result.state=detail::export_state(sim); result.hires=result.state.worker_count-1;
    for(int p=0;p<N_PRODUCTS;++p) result.production[p]=sim.st.farms[0].produced[p];
    result.status=SolveStatus::Success; auto check=verify(in,result);
    if(!check.valid) result.status=SolveStatus::NoScheduleFound;
    result.microseconds=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-began).count();
    return check;
}
}
