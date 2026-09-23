#include "remaining.hpp"
#include <algorithm>
#include <climits>

namespace kag::day_compiler {
namespace {
int event(int op) {
    switch(op) {
        case OP_WATER:return worker::Water;
        case OP_FERTILIZE:return worker::Fertilize;
        case OP_HARVEST:return worker::Harvest;
        case OP_DIG:return worker::Clear;
        case OP_FEED:return worker::Feed;
        case OP_CARE:return worker::Care;
        case OP_COLLECT_FERTILIZER:return worker::CollectFertilizer;
        default:return 0;
    }
}
}
bool RemainingWork::begin(const Observation& o,const worker::DayInput& input,const worker::SolveOptions& constraints) {
    if(input.start_hour!=o.hour || input.worker_count!=o.self().n_units || input.worker_count<1 || input.worker_count>MAX_UNITS ||
       input.establish_count<0 || input.establish_count>100 || input.start_hour<0 || input.start_hour>=input.hours || input.hours>24) return false;
    for(int a=0;a<input.establish_count;++a) for(int b=0;b<a;++b)
        if(input.establish[a].product==input.establish[b].product && input.establish[a].events!=input.establish[b].events) return false;
    original_=input; constraints_=constraints; day_=o.day; next_hour_=o.hour;
    target_quadrants_=o.self().n_quadrants+int(input.land_hour>=0);
    std::copy_n(input.events,100,events_); std::fill_n(established_,100,false); std::fill_n(receipts_,N_PRODUCTS,0);
    return true;
}
bool RemainingWork::record(const Observation& o,const Action& action,const Configuration& config) {
    if(o.day!=day_ || o.hour!=next_hour_ || action.n_units!=o.self().n_units) return false;
    Config engine_config; engine_config.shed_capacity=config.shed_capacity;
    Sim sim(engine_config); sim.st.day=o.day; sim.st.farms[o.player]=own_farm(o);
    auto requested=action; requested.n_orders=0; requested.finalize();
    const auto accepted=sim.sanitize_solo_action(o.player,requested);
    Tile tiles[100]; std::copy_n(&o.self().tiles[0][0],100,tiles);
    int stock[N_ITEMS],total=o.own.shed_total; std::copy_n(o.own.shed,N_ITEMS,stock);
    auto deposit=[&](int p,int n) {
        const int admitted=std::min(n,config.shed_capacity-total);
        stock[p]+=admitted; total+=admitted; if(p<N_PRODUCTS)receipts_[p]+=admitted;
    };
    for(int u=0;u<accepted.n_units;++u) {
        const auto a=accepted.units[u]; const int cell=o.self().pos_y[u]*10+o.self().pos_x[u]; auto& tile=tiles[cell];
        const bool placement=a.op==OP_PLACE && is_animal(a.arg) && !tile.has_animal &&
            tile.kind==(ANIMALS[a.arg-GOOSE].structure==ST_COOP?T_COOP:T_PASTURE);
        if(a.op==OP_PLANT || placement) {
            if(events_[cell]) return false;
            int j=0;
            while(j<original_.establish_count && (established_[j] || original_.establish[j].product!=a.arg))++j;
            if(j==original_.establish_count) return false;
            established_[j]=true; events_[cell]=original_.establish[j].events;
            tile.kind=placement?tile.kind:T_PLANT; tile.has_animal=placement; tile.what=a.arg;
        } else if(a.op==OP_PICKUP) {
            const int quantity=std::min(a.n,stock[a.arg]); stock[a.arg]-=quantity; total-=quantity;
        } else if(a.op==OP_PLACE) {
            deposit(a.arg,std::min<int>(a.n,o.own.inv[u][a.arg]));
        } else if(a.op==OP_DROP) {
            for(int k=0;k<o.own.inv_nkeys[u];++k) { const int p=o.own.inv_keys[u][k]; deposit(p,o.own.inv[u][p]); }
        } else if(a.op==OP_BUILD_COOP || a.op==OP_BUILD_PASTURE) {
            tile.kind=a.op==OP_BUILD_COOP?T_COOP:T_PASTURE;
        } else {
            const int e=event(a.op);
            if(e && !(events_[cell]&e) && e!=worker::Clear) return false;
            events_[cell]&=~e;
            if(e==worker::Clear || (e==worker::Harvest && tile.kind==T_PLANT && !CROPS[tile.what].ongoing)) {
                events_[cell]&=~worker::Clear; tile=Tile{};
            }
        }
    }
    ++next_hour_; return true;
}
bool RemainingWork::build(const Observation& o,Workload& out,bool direct_field_inputs) const {
    if(o.day!=day_ || o.hour!=next_hour_ || o.hour>=original_.hours || o.self().n_quadrants>target_quadrants_) return false;
    out=Workload{}; out.input=original_; out.constraints=constraints_;
    auto& in=out.input; in.start_hour=o.hour; in.worker_count=o.self().n_units;
    std::copy_n(o.own.shed,N_ITEMS,in.shed); std::copy_n(o.own.seeds,N_CROPS,in.seeds);
    int carried[N_ITEMS]{};
    for(int u=0;u<in.worker_count;++u) {
        auto& worker=in.workers[u]; worker.tile=o.self().pos_y[u]*10+o.self().pos_x[u];
        std::copy_n(o.own.inv[u],N_ITEMS,worker.inventory); worker.inventory_count=o.own.inv_nkeys[u];
        std::copy_n(o.own.inv_keys[u],worker.inventory_count,worker.inventory_keys);
        for(int p=0;p<N_ITEMS;++p)carried[p]+=worker.inventory[p];
    }
    int feeds=0,fertilizers=0;
    for(int c=0;c<100;++c) {
        const auto& tile=o.self().tiles[c/10][c%10]; auto& normalized=in.grid[c];
        normalized=tile; normalized.planted_day-=o.day; normalized.fertilized_until_day-=o.day;
        normalized.max_lifespan_step=tile.max_lifespan_step<0?INT_MAX:tile.max_lifespan_step-o.day*24;
        const int e=in.events[c]=events_[c]; feeds+=bool(e&worker::Feed); fertilizers+=bool(e&worker::Fertilize);
        if(e&worker::Harvest) {
            const int product=tile.has_animal?int(ANIMALS[tile.what-GOOSE].product):int(tile.what);
            out.harvest_amount[c]=planned_harvest_yield(tile,o.day,e); out.field_output[product]+=out.harvest_amount[c];
        }
        if(e&worker::CollectFertilizer)++out.field_output[FERTILIZER];
    }
    in.establish_count=0;
    for(int j=0;j<original_.establish_count;++j) if(!established_[j]) {
        const auto n=original_.establish[j]; in.establish[in.establish_count++]=n;
        if(is_crop(n.product))++out.seed_need[n.product]; else ++out.animal_need[n.product-GOOSE];
        feeds+=bool(n.events&worker::Feed); fertilizers+=bool(n.events&worker::Fertilize);
    }
    for(int p=0;p<N_CROPS;++p)out.seed_need[p]=std::max(0,out.seed_need[p]-in.seeds[p]);
    for(int a=0;a<3;++a)out.animal_need[a]=std::max(0,out.animal_need[a]-in.shed[GOOSE+a]-carried[GOOSE+a]);
    const int direct_wheat=direct_field_inputs?std::min(out.field_output[WHEAT],std::max(0,feeds-in.shed[WHEAT]-carried[WHEAT])):0;
    const int direct_fertilizer=direct_field_inputs?std::min(out.field_output[FERTILIZER],std::max(0,fertilizers-in.shed[FERTILIZER]-carried[FERTILIZER])):0;
    out.wheat_need=std::max(0,feeds-in.shed[WHEAT]-carried[WHEAT]-direct_wheat);
    out.fertilizer_need=std::max(0,fertilizers-in.shed[FERTILIZER]-carried[FERTILIZER]-direct_fertilizer);
    out.field_wheat_used=direct_wheat+std::min(carried[WHEAT],std::max(0,feeds-in.shed[WHEAT]));
    out.field_fertilizer_used=direct_fertilizer+std::min(carried[FERTILIZER],std::max(0,fertilizers-in.shed[FERTILIZER]));
    for(int p=0;p<N_PRODUCTS;++p)out.field_output[p]+=carried[p];
    for(int h=0;h<24;++h) {
        if(h<o.hour) {
            std::fill_n(in.buy_seeds[h],N_CROPS,0); std::fill_n(in.buy_animals[h],3,0);
            in.buy_wheat[h]=in.buy_fertilizer[h]=0;
        }
        for(int p=0;p<N_PRODUCTS;++p)in.returns[h][p]=h<o.hour?0:std::max(0,original_.returns[h][p]-receipts_[p]);
    }
    if(!needs_land(o))in.land_hour=-1;
    else in.land_hour=std::max(in.land_hour,int(o.hour));
    return true;
}
}
