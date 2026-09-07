#include "agent.hpp"
#include "upstream/policy.hpp"
#include "../../../include/finance7_layer.hpp"

namespace compositions::destbreso_finance7 {

void AgentCore::reset(const kag::agent::AgentInit& init) {
    config_ = kag::Config{};
    config_.episode_steps = init.config.episode_steps;
    config_.max_orders = init.config.max_orders;
    config_.hire_mult = init.config.hire_mult;
    route_ = streak_ = finance_fires_ = mirror_fires_ = 0;
    mirror_route_=-1; scores_.fill(0); is_mirror_=false;
}

void AgentCore::act(const kag::agent::AgentObservation& o,
                const kag::agent::DecisionBudget&, kag::Action& action) {
    kag::State state{};
    state.step = o.step;
    state.day = o.day;
    state.hour = o.hour;
    state.market = o.market;
    state.n_shops = o.n_shops;
    std::copy_n(o.shops, o.n_shops, state.shops);
    for (int seat = 0; seat < 2; ++seat) {
        auto& f = state.farms[seat];
        const auto& p = o.farms[seat];
        f.money = p.money;
        f.n_units = p.n_units;
        f.n_quadrants = p.n_quadrants;
        f.hires_today = p.hires_today;
        for (int y = 0; y < kag::BOARD; ++y)
            for (int x = 0; x < kag::BOARD; ++x) f.tiles[y][x] = p.tiles[y][x];
        std::copy_n(p.pos_x, p.n_units, f.pos_x);
        std::copy_n(p.pos_y, p.n_units, f.pos_y);
    }
    auto& own = state.farms[o.player];
    own.shed_total = o.own.shed_total;
    std::copy_n(o.own.shed, kag::N_ITEMS, own.shed);
    for (int u = 0; u < own.n_units; ++u)
        std::copy_n(o.own.inv[u], kag::N_ITEMS, own.inv[u]);
    Context context;
    context.selected_route = route_;
    action = context.act(state, config_, o.player);
    route_ = context.selected_route;

    if(mode_&1)mirror(o,action);
    if((mode_&2) && finance7_hires(o,action))++finance_fires_;
    for(int u=action.n_units;u<own.n_units;++u)action.units[u]={};
    action.n_units=own.n_units;action.finalize();
}

namespace {
struct Sales {
    std::array<kag::Order,10> orders{};
    int count=0;
    bool operator==(const Sales& other) const {
        if(count!=other.count)return false;
        for(int i=0;i<count;++i)
            if(orders[i].item!=other.orders[i].item || orders[i].n!=other.orders[i].n)return false;
        return true;
    }
};
Sales sales(const kag::Action& action) {
    Sales result;
    for(int i=0;i<action.n_orders;++i)
        if(action.orders[i].op==kag::M_SELL && action.orders[i].item!=kag::WHEAT)
            result.orders[result.count++]=action.orders[i];
    return result;
}
std::array<long long,5> signature(const kag::agent::PublicFarm& farm) {
    int plants=0,animals=0;
    for(const auto& row:farm.tiles)for(const auto& tile:row) {
        plants+=tile.kind==kag::T_PLANT;animals+=tile.has_animal;
    }
    return {std::llround(farm.money*100),farm.n_quadrants,farm.n_units-1,plants,animals};
}
}

void AgentCore::mirror(const kag::agent::AgentObservation& o,kag::Action& action) {
    if(!is_mirror_ && o.step>=100 && o.step<=143) {
        if(signature(o.self())==signature(o.opponent())) {
            if(++streak_>=20)is_mirror_=true;
        } else streak_=0;
    }
    const auto& courses=Context::decoded_actions();
    if(mirror_route_<0 && o.step>=360) {
        const auto zero=sales(courses[0][o.step]),one=sales(courses[1][o.step]);
        if(!(zero==one)) {
            const auto own=sales(action);
            scores_[0]+=own==zero;scores_[1]+=own==one;
            if(scores_[0]!=scores_[1] && std::max(scores_[0],scores_[1])>=2)
                mirror_route_=scores_[0]>scores_[1]?0:1;
        }
    }
    if(!is_mirror_ || o.step<144 || o.step>716)return;
    const auto zero=sales(courses[0][o.step+1]),one=sales(courses[1][o.step+1]);
    int route=0;
    if(o.step+1>=360 && !(zero==one)) {
        if(mirror_route_<0)return;
        route=mirror_route_;
    }
    const auto selected=route?one:zero;bool changed=false;
    for(int i=0;i<selected.count && action.n_orders<10;++i) {
        action.orders[action.n_orders++]=selected.orders[i];changed=true;
    }
    mirror_fires_+=changed;
}
}
