#pragma once
#include <array>
#include <bit>
#include <memory>
#include <ostream>
#include <vector>
#include "fast_game_engine/sim.hpp"

namespace compositions {
struct ObservedLife {
    int item=0, x=0, y=0, start=0, end=719, born_day=0;
    uint32_t days=0, water=0, fed=0, cared=0, fertilizer=0, fertilized=0;
};

struct FarmProfile {
    struct Flow { int step,item,bought,sold; };
    struct StockFlow { int step,item,arrived,withdrawn; };
    struct FixedCost { int step,cost,orders; };
    std::vector<ObservedLife> lives;
    std::vector<Flow> flows;
    std::vector<StockFlow> stocks;
    std::vector<FixedCost> fixed;
    std::array<int,100> active;
    int successful[kag::OP_INVALID]{}, requested[kag::OP_INVALID]{};
    int buys[kag::N_ITEMS]{}, seed_buys[kag::N_CROPS]{};
    int buy_hours[24][kag::N_PRODUCTS]{}, sell_hours[24][kag::N_PRODUCTS]{};
    int hires=0, hire_cost=0, land=0, land_cost=0, weed_digs=0, shed_max=0, shed_ge90=0;
    FarmProfile() { active.fill(-1); lives.reserve(300); flows.reserve(500); }
    void close(int cell, int step) {
        if (active[cell]>=0) lives[active[cell]].end=step;
        active[cell]=-1;
    }
};

struct DetailedProfile {
    FarmProfile farms[2];

    static int portable(const kag::Farm& f,int item) {
        int total=f.shed[item];
        for (int u=0;u<f.n_units;++u) total+=f.inv[u][item];
        return total;
    }

    void observe(const kag::Sim& before,const kag::Sim& after,const kag::Action (&actions)[2]) {
        using namespace kag;
        // On day-end turns retain the exact pre-refresh farm. Only st.hour is
        // changed: unit processing uses day, market/town/decay use step. This
        // suppresses end_of_day in a diagnostic copy, never in the real game.
        std::unique_ptr<Sim> phase;
        if (before.st.hour==23) {
            phase=std::make_unique<Sim>(before);
            phase->st.hour=0;
            phase->step(actions[0],actions[1]);
            for(int p=0;p<2;++p)
                if(phase->st.farms[p].money!=after.st.farms[p].money) std::abort();
        }
        const int day=before.st.day, hour=before.st.hour, result_step=before.st.step+1;
        const uint32_t bit=uint32_t{1}<<day;
        for(int p=0;p<2;++p) {
            auto& report=farms[p];
            const auto& old=before.st.farms[p];
            const auto& now=after.st.farms[p];
            const auto& worked=phase ? phase->st.farms[p] : now;
            auto only_units=actions[p]; only_units.n_orders=0;
            const auto accepted=before.sanitize_solo_action(p,only_units);
            int consumed[N_ITEMS]{}, planted[N_CROPS]{}, deposited[N_ITEMS]{}, withdrawn[N_ITEMS]{};
            int fixed_cost=0,fixed_orders=0;
            TileKind kinds[100];
            for(int y=0;y<BOARD;++y) for(int x=0;x<BOARD;++x) {
                const int cell=y*BOARD+x;
                kinds[cell]=old.tiles[y][x].kind;
                if(report.active[cell]>=0) {
                    auto& life=report.lives[report.active[cell]];
                    life.days|=bit;
                    if(old.tiles[y][x].kind==T_PLANT && old.tiles[y][x].fertilized_until_day>=day) life.fertilized|=bit;
                }
            }
            for(int u=0;u<old.n_units;++u) {
                const auto requested=actions[p].units[u];
                if(requested.op!=OP_PASS) ++report.requested[requested.op];
                const auto a=accepted.units[u];
                if(a.op==OP_PASS) continue;
                ++report.successful[a.op];
                const int x=old.pos_x[u],y=old.pos_y[u],cell=y*BOARD+x;
                int id=report.active[cell];
                const bool placed_animal=a.op==OP_PLACE && is_animal(a.arg) && id<0 &&
                    kinds[cell]==(a.arg==GOOSE ? T_COOP : T_PASTURE);
                if(a.op==OP_PLANT || placed_animal) {
                    if(id>=0) std::abort();
                    report.active[cell]=id=report.lives.size();
                    report.lives.push_back({a.arg,x,y,result_step,719,day,bit});
                    if(a.op==OP_PLANT) { ++planted[a.arg]; kinds[cell]=T_PLANT; }
                    else ++consumed[a.arg];
                }
                if(a.op==OP_FEED) ++consumed[WHEAT];
                if(a.op==OP_FERTILIZE) ++consumed[FERTILIZER];
                if(a.op==OP_PICKUP) withdrawn[a.arg]+=worked.inv[u][a.arg]-old.inv[u][a.arg];
                if(a.op==OP_DROP) for(int item=0;item<N_ITEMS;++item) deposited[item]+=old.inv[u][item]-worked.inv[u][item];
                if(a.op==OP_PLACE && !placed_animal) deposited[a.arg]+=old.inv[u][a.arg]-worked.inv[u][a.arg];
                if(a.op==OP_DIG && kinds[cell]==T_WEED) ++report.weed_digs;
                if(id>=0) {
                    auto& life=report.lives[id];
                    if(a.op==OP_WATER) life.water|=bit;
                    if(a.op==OP_FEED) life.fed|=bit;
                    if(a.op==OP_CARE) life.cared|=bit;
                    if(a.op==OP_COLLECT_FERTILIZER) life.fertilizer|=bit;
                    if(a.op==OP_FERTILIZE) life.fertilized|=bit;
                    if(a.op==OP_DIG || (a.op==OP_HARVEST && is_crop(life.item) && !CROPS[life.item].ongoing))
                        report.close(cell,result_step);
                }
                if(a.op==OP_DIG) kinds[cell]=T_EMPTY;
                if(a.op==OP_BUILD_COOP) kinds[cell]=T_COOP;
                if(a.op==OP_BUILD_PASTURE) kinds[cell]=T_PASTURE;
            }
            for(int y=0;y<BOARD;++y) for(int x=0;x<BOARD;++x) {
                const int cell=y*BOARD+x,id=report.active[cell];
                if(id<0) continue;
                auto& life=report.lives[id];
                const auto& tile=worked.tiles[y][x];
                if(tile.kind==T_PLANT && tile.fertilized_until_day>=day) life.fertilized|=bit;
                const auto& next=now.tiles[y][x];
                if(!(next.kind==T_PLANT || next.has_animal) || next.what!=life.item || next.planted_day!=life.born_day)
                    report.close(cell,result_step);
            }
            for(int item=0;item<N_ITEMS;++item) {
                const int sold=now.sold_units[item]-old.sold_units[item];
                const int bought=portable(now,item)-portable(old,item)
                    -(now.produced[item]-old.produced[item])+sold+consumed[item]
                    +(now.discarded[item]-old.discarded[item]);
                if(bought<0) std::abort();
                report.buys[item]+=bought;
                if(item<N_PRODUCTS) {
                    report.buy_hours[hour][item]+=bought; report.sell_hours[hour][item]+=sold;
                    if(bought || sold) report.flows.push_back({before.st.step,item,bought,sold});
                    const int arrived=deposited[item]-(worked.discarded[item]-old.discarded[item]);
                    if(arrived<0 || withdrawn[item]<0) std::abort();
                    if(worked.shed[item]!=old.shed[item]+arrived-withdrawn[item]+bought-sold) std::abort();
                    if(arrived || withdrawn[item]) report.stocks.push_back({before.st.step,item,arrived,withdrawn[item]});
                    if(phase) {
                        const int night_arrival=now.shed[item]-worked.shed[item];
                        if(night_arrival<0) std::abort();
                        if(night_arrival) report.stocks.push_back({result_step,item,night_arrival,0});
                    }
                } else if(bought) {
                    fixed_cost+=bought*ANIMALS[item-GOOSE].cost;++fixed_orders;
                }
            }
            for(int item=0;item<N_CROPS;++item) {
                const int bought=now.seeds[item]-old.seeds[item]+planted[item];
                if(bought<0) std::abort();
                report.seed_buys[item]+=bought;
                if(bought) {fixed_cost+=bought*CROPS[item].seed;++fixed_orders;}
            }
            for(int hire=old.hires_today;hire<worked.hires_today;++hire) {
                ++report.hires; report.hire_cost+=before.cfg.hire_mult*fib(hire);
                fixed_cost+=before.cfg.hire_mult*fib(hire);++fixed_orders;
            }
            for(int n=old.n_quadrants;n<now.n_quadrants;++n) {
                ++report.land; report.land_cost+=LAND_PRICES[n-1];
                fixed_cost+=LAND_PRICES[n-1];++fixed_orders;
            }
            if(fixed_cost) report.fixed.push_back({before.st.step,fixed_cost,fixed_orders});
            if(now.total_spend-old.total_spend<fixed_cost) std::abort();
            report.shed_max=std::max(report.shed_max,old.shed_total);
            report.shed_ge90+=old.shed_total>=90;
        }
    }

    void write(std::ostream& out,int p) const {
        const auto& f=farms[p];
        out << "{\"hires\":" << f.hires << ",\"hire_cost\":" << f.hire_cost
            << ",\"land\":" << f.land << ",\"land_cost\":" << f.land_cost
            << ",\"weed_digs\":" << f.weed_digs << ",\"shed_max\":" << f.shed_max << ",\"shed_ge90\":" << f.shed_ge90;
        auto array=[&](const char* key,const auto& a,int n) {
            out << ",\"" << key << "\":[";
            for(int i=0;i<n;++i) { if(i) out << ','; out << a[i]; }
            out << ']';
        };
        array("buys",f.buys,kag::N_ITEMS); array("seed_buys",f.seed_buys,kag::N_CROPS);
        array("successful",f.successful,kag::OP_INVALID); array("requested",f.requested,kag::OP_INVALID);
        auto hours=[&](const char* key,const auto& a) {
            out << ",\"" << key << "\":[";
            for(int h=0;h<24;++h) { if(h)out<<','; out<<'[';
                for(int i=0;i<kag::N_PRODUCTS;++i) {if(i)out<<',';out<<a[h][i];} out<<']'; }
            out<<']';
        };
        hours("buy_hours",f.buy_hours); hours("sell_hours",f.sell_hours);
        out << ",\"lives\":[";
        for(size_t i=0;i<f.lives.size();++i) {
            if(i)out<<',';const auto& l=f.lives[i];
            out << '[' << l.item << ',' << l.x << ',' << l.y << ',' << l.start << ',' << l.end << ',' << l.born_day
                << ',' << l.days << ',' << l.water << ',' << l.fed << ',' << l.cared << ',' << l.fertilizer << ',' << l.fertilized << ']';
        }
        out << "],\"flows\":[";
        for(size_t i=0;i<f.flows.size();++i) {
            if(i)out<<',';const auto& flow=f.flows[i];
            out<<'['<<flow.step<<','<<flow.item<<','<<flow.bought<<','<<flow.sold<<']';
        }
        out << "],\"stock_flows\":[";
        for(size_t i=0;i<f.stocks.size();++i) {
            if(i)out<<',';const auto& flow=f.stocks[i];
            out<<'['<<flow.step<<','<<flow.item<<','<<flow.arrived<<','<<flow.withdrawn<<']';
        }
        out << "],\"fixed_costs\":[";
        for(size_t i=0;i<f.fixed.size();++i) {
            if(i)out<<',';const auto& cost=f.fixed[i];
            out<<'['<<cost.step<<','<<cost.cost<<','<<cost.orders<<']';
        }
        out << "]}";
    }
};
}
