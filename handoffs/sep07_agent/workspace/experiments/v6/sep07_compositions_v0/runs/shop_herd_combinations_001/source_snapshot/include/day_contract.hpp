#pragma once
#include "evaluation.hpp"
#include "../league/top_replay_library/source/agent.hpp"
#include "../league/public_router/source/agent.hpp"
#include "day_solver/scheduler.hpp"
#include "day_solver/io.hpp"
#include <filesystem>
#include <iostream>

namespace compositions::day_contract {
using namespace compositions;
using namespace kag;
using namespace day_solver;

ManagedTileState managed(const Tile& tile,int day) {
    ManagedTileState result;
    constexpr ManagedTileKind kinds[]={ManagedTileKind::EMPTY,ManagedTileKind::LOCKED,ManagedTileKind::WEED,
        ManagedTileKind::COOP,ManagedTileKind::PASTURE,ManagedTileKind::CROP};
    result.kind=kinds[tile.kind];
    if(tile.kind==T_PLANT || tile.has_animal) {
        result.age_days=day-tile.planted_day;
        result.stored_units=tile.yield_units;result.consecutive_dry_days=tile.consecutive_dry;
    }
    if(tile.kind==T_PLANT) {
        result.crop=tile.what;result.watered_today=tile.watered_today;
        result.fertilizer_days_remaining=std::max(0,tile.fertilized_until_day-day+1);
    }
    if(tile.has_animal) {
        result.animal=tile.what;result.fed_today=tile.fed_today;result.cared_today=tile.cared_today;
        result.pending_care_bonus=tile.pending_care_bonus;result.fertilizer_available=tile.fertilizer_available;
    }
    return result;
}

// Offline source-contract extraction only. Prefix probes repeat the same
// pre-turn state; no copied opponent state is passed to an executable policy.
Sim prefix_phase(const Sim& before,const Action (&actions)[2],int orders,int workers=-1) {
    Sim phase=before;
    if(phase.st.hour==23)phase.st.hour=0;
    Action pair[2]={actions[0],actions[1]};
    for(auto& action:pair) {
        action.n_orders=std::min(action.n_orders,orders);
        if(workers>=0)for(int u=workers;u<action.n_units;++u)action.units[u]={};
        action.finalize();
    }
    phase.step(pair[0],pair[1]);
    return phase;
}

struct RecordedDay {
    Sim start;
    std::array<Action,24> own,rival;
    DayProblem problem;
    int discarded=0;
    explicit RecordedDay(const Sim& sim):start(sim) {
        const auto& farm=sim.st.farms[0];
        for(int y=0;y<10;++y)for(int x=0;x<10;++x)
            problem.start.managed_tiles.push_back({int8_t(x),int8_t(y),managed(farm.tiles[y][x],sim.st.day)});
        std::copy_n(farm.shed,N_ITEMS,problem.start.shed.begin());
        std::copy_n(farm.seeds,N_CROPS,problem.start.seeds.begin());
    }
};

void save_actions(const std::array<Action,24>& actions,const std::filesystem::path& path) {
    std::ofstream file(path);
    for(const auto& action:actions) {
        file<<action.n_units<<' '<<action.n_orders;
        for(int u=0;u<action.n_units;++u)file<<' '<<+action.units[u].op<<' '<<+action.units[u].arg<<' '<<action.units[u].n;
        for(int i=0;i<action.n_orders;++i)file<<' '<<+action.orders[i].op<<' '<<+action.orders[i].item<<' '<<action.orders[i].n;
        file<<'\n';
    }
}

void append_contract(RecordedDay& day,const Sim& before,const Sim& after,const Action (&actions)[2]) {
    const int hour=before.st.hour;
    day.own[hour]=actions[0];day.rival[hour]=actions[1];
    auto& problem=day.problem;
    Action no_market=actions[0];no_market.n_orders=0;
    const auto accepted=before.sanitize_solo_action(0,no_market);
    std::copy_n(accepted.units,accepted.n_units,day.own[hour].units);
    day.own[hour].finalize();
    Action physical[2]={accepted,actions[1]};
    physical[1].n_orders=0;
    auto previous=prefix_phase(before,physical,0,0);
    for(int u=0;u<before.st.farms[0].n_units;++u) {
        auto current=prefix_phase(before,physical,0,u+1);
        const auto action=accepted.units[u];
        const int x=before.st.farms[0].pos_x[u],y=before.st.farms[0].pos_y[u],cell=y*10+x;
        const auto& tile=previous.st.farms[0].tiles[y][x];
        const bool place=action.op==OP_PLACE && is_animal(action.arg) && !tile.has_animal &&
            tile.kind==(action.arg==GOOSE?T_COOP:T_PASTURE);
        const bool field=place || (action.op>=OP_PLANT && action.op<=OP_CARE);
        if(field) {
            auto found=std::find_if(problem.tile_work.begin(),problem.tile_work.end(),[&](const TileWork& work){return work.tile==cell;});
            if(found==problem.tile_work.end()) {problem.tile_work.push_back({int16_t(cell),{}});found=problem.tile_work.end()-1;}
            TileWorkAction work;work.op=action.op;
            if(action.op==OP_PLANT || place)work.arg=action.arg;
            if(action.op==OP_COLLECT_FERTILIZER) {work.output_item=FERTILIZER;work.output_quantity=1;}
            if(action.op==OP_HARVEST) {
                const auto& a=previous.st.farms[0];const auto& b=current.st.farms[0];
                int item=-1,quantity=0;
                for(int i=0;i<N_PRODUCTS;++i)if(b.produced[i]>a.produced[i]) {item=i;quantity+=b.produced[i]-a.produced[i];}
                if(item<0 || quantity<=0)std::abort();
                work.arg=work.output_item=item;work.output_quantity=quantity;
            }
            found->actions.push_back(work);
        }
        previous=std::move(current);
    }
    // Exact accepted orders with their original slots, including simultaneous
    // rival quotes. Public v3 withdrawals require sales before that hour's buys;
    // the original schedule replay below detects unsupported same-hour reuse.
    auto prior=prefix_phase(before,actions,0);
    std::array<int64_t,N_ITEMS> sold{};
    for(int i=0;i<actions[0].n_orders;++i) {
        auto current=prefix_phase(before,actions,i+1);
        const auto& a=prior.st.farms[0];const auto& b=current.st.farms[0];
        const auto order=actions[0].orders[i];
        MarketEvent event;event.hour=hour;event.order_index=i;event.market_op=order.op;
        int quantity=0;
        if(order.op==M_HIRE) {quantity=b.n_units-a.n_units;event.item=-1;problem.worker_count+=quantity;}
        if(order.op==M_BUY_LAND) {quantity=b.n_quadrants-a.n_quadrants;event.item=a.n_quadrants;}
        if(order.op==M_BUY_SEED) {quantity=b.seeds[order.item]-a.seeds[order.item];event.item=order.item;}
        if(order.op==M_BUY_PRODUCT || order.op==M_BUY_ANIMAL) {quantity=b.shed[order.item]-a.shed[order.item];event.item=order.item;}
        if(order.op==M_SELL)sold[order.item]+=b.sold_units[order.item]-a.sold_units[order.item];
        if(quantity>0) {event.quantity=quantity;problem.market_plan.push_back(event);}
        // The semantic contract contains accepted quantities. Reusing a larger
        // failed request could buy/sell more after worker routes are changed.
        const int committed=order.op==M_SELL?b.sold_units[order.item]-a.sold_units[order.item]:quantity;
        day.own[hour].orders[i]=committed>0?Order{order.op,order.item,committed}:Order{};
        prior=std::move(current);
    }
    // Workers cannot act between market slots. Cancel same-hour product buys
    // and sales only in the physical scheduler contract, whose withdrawal
    // phase precedes buys. Preserve every accepted trade in day.own for exact
    // cash/price replay. This keeps opening wheat round trips out of field-work
    // requirements without erasing their economic effects.
    for(auto& event:problem.market_plan)
        if(event.hour==hour && event.market_op==M_BUY_PRODUCT) {
            const int64_t cancel=std::min<int64_t>(event.quantity,sold[event.item]);
            event.quantity-=cancel;sold[event.item]-=cancel;
        }
    std::erase_if(problem.market_plan,[](const MarketEvent& event){return event.quantity==0;});
    for(int item=0;item<N_ITEMS;++item) {
        problem.shed_availability[hour][item]=(hour?problem.shed_availability[hour-1][item]:0)+sold[item];
        day.discarded+=after.st.farms[0].discarded[item]-before.st.farms[0].discarded[item];
    }
    if(hour==23) {
        const auto& farm=after.st.farms[0];
        std::copy_n(farm.shed,N_ITEMS,problem.end_shed.begin());
        std::copy_n(farm.seeds,N_CROPS,problem.end_seeds.begin());
        for(int cell=0;cell<100;++cell) {
            auto tile=farm.tiles[cell/10][cell%10];
            // Remove only stochastic weeds born on previously empty squares.
            if(tile.kind==T_WEED && prior.st.farms[0].tiles[cell/10][cell%10].kind==T_EMPTY)tile={};
            EndTileRequirement end;end.tile=cell;end.exact_state=managed(tile,after.st.day);
            problem.required_end_tiles.push_back(end);
        }
    }
}


}
