#pragma once
#include "agents/common/api/agent_api.hpp"

namespace compositions::day_programs {

// A clock-only simulator aligns deterministic consumption timers. It never
// receives the actual episode seed or any rival-private inventory.
class ObservationModel {
    kag::Sim clock_;
    static void public_farm(const kag::agent::PublicFarm& source,kag::Farm& target) {
        target=kag::Farm{};
        target.money=source.money;target.n_units=source.n_units;
        target.n_quadrants=source.n_quadrants;target.hires_today=source.hires_today;
        std::copy_n(source.pos_x,kag::MAX_UNITS,target.pos_x);
        std::copy_n(source.pos_y,kag::MAX_UNITS,target.pos_y);
        for(int cell=0;cell<100;++cell) {
            const auto& tile=source.tiles[cell/10][cell%10];
            target.tiles[cell/10][cell%10]=tile;
            const int word=cell/64;const uint64_t bit=uint64_t{1}<<(cell%64);
            if(tile.kind==kag::T_EMPTY)target.empty_mask[word]|=bit;
            if(tile.kind==kag::T_PLANT) {
                target.plant_mask[word]|=bit;
                if(tile.max_lifespan_step>=0) {
                    target.decay_mask[word]|=bit;
                    target.next_decay_step=std::min(target.next_decay_step,int(tile.max_lifespan_step));
                }
            }
            if(tile.has_animal)target.animal_mask[word]|=bit;
        }
    }
public:
    void reset(const kag::agent::AgentConfig& c) {
        kag::Config cfg;
        cfg.seed=0;cfg.weed_chance=0;
        cfg.episode_steps=c.episode_steps;cfg.board_size=c.board_size;cfg.starting_money=c.starting_money;
        cfg.max_orders=c.max_orders;cfg.turns_per_day=c.turns_per_day;cfg.shed_capacity=c.shed_capacity;
        cfg.shop_unlock_interval=c.shop_unlock_interval;cfg.shop_sell_interval=c.shop_sell_interval;
        cfg.center_sell_interval=c.center_sell_interval;cfg.hire_mult=c.hire_mult;
        clock_=kag::Sim(cfg);
    }
    void advance(int step) {
        if(step<clock_.st.step)std::abort();
        while(clock_.st.step<step) {
            kag::Action pass[2];
            for(int p=0;p<2;++p){pass[p].clear();pass[p].n_units=clock_.st.farms[p].n_units;pass[p].finalize();}
            clock_.step(pass[0],pass[1]);
        }
    }
    kag::Sim make(const kag::agent::AgentObservation& o) const {
        if(clock_.st.step!=o.step)std::abort();
        auto sim=clock_;
        sim.st.market=o.market;sim.st.n_shops=o.n_shops;
        std::fill(std::begin(sim.st.shops),std::end(sim.st.shops),0);
        std::copy_n(o.shops,o.n_shops,sim.st.shops);
        for(int p=0;p<2;++p)public_farm(o.farms[p],sim.st.farms[p]);
        auto& farm=sim.st.farms[o.player];
        std::copy_n(o.own.shed,kag::N_ITEMS,farm.shed);farm.shed_total=o.own.shed_total;
        std::copy_n(o.own.seeds,kag::N_CROPS,farm.seeds);
        for(int u=0;u<o.self().n_units;++u) {
            std::copy_n(o.own.inv[u],kag::N_ITEMS,farm.inv[u]);
            farm.inv_nkeys[u]=o.own.inv_nkeys[u];
            std::copy_n(o.own.inv_keys[u],o.own.inv_nkeys[u],farm.inv_keys[u]);
        }
        return sim;
    }
};
}
