#include "history.hpp"
#include <algorithm>

namespace kag::day_compiler {
void History::record(const Observation& observation, const Farm& after_workers, const Action& action) {
    pending_ = observation; after_workers_ = after_workers; action_ = action;
    pending_step_ = observation.step;
}
bool History::observe(const Observation& o, const Configuration& c) {
    if (pending_step_ < 0 || o.step != pending_step_ + 1 || o.player != pending_.player) {
        pending_step_ = -1;
        return false;
    }
    last_ = pending_step_ % samples_.size(); auto& sample = samples_[last_]; sample = {};
    sample.step = pending_step_;
    int previous_held[N_PRODUCTS]{},current_held[N_PRODUCTS]{};
    for(int cell=0;cell<BOARD*BOARD;++cell) {
        const auto& previous=pending_.opponent().tiles[cell/BOARD][cell%BOARD];
        const auto& current=o.opponent().tiles[cell/BOARD][cell%BOARD];
        if(previous.kind==T_PLANT) previous_held[previous.what]+=previous.yield_units;
        else if(previous.has_animal) previous_held[ANIMALS[previous.what-GOOSE].product]+=previous.yield_units;
        if(current.kind==T_PLANT) current_held[current.what]+=current.yield_units;
        else if(current.has_animal) current_held[ANIMALS[current.what-GOOSE].product]+=current.yield_units;
        if(previous.has_animal && current.has_animal && previous.what==current.what &&
           previous.fertilizer_available && !current.fertilizer_available)
            ++previous_held[FERTILIZER];
    }
    for (int product = 0; product < N_PRODUCTS; ++product) {
        sample.rival_visible_removal[product]=std::max(0,previous_held[product]-current_held[product]);
        int cargo = 0;
        for (int unit = 0; unit < after_workers_.n_units; ++unit) cargo += after_workers_.inv[unit][product];
        const bool night = o.day != pending_.day;
        const int arrival = night ? cargo : 0;
        // Full night settlement may discard cargo. Do not invent a fill then.
        const bool fill_known = !night || !cargo || o.own.shed_total < c.shed_capacity;
        const int net = after_workers_.shed[product] + arrival - o.own.shed[product];
        sample.own_fill_known[product] = fill_known;
        sample.own_net_sales[product] = fill_known ? net : 0;
        int requested_buys = 0, requested_sales = 0;
        for (int slot = 0; slot < std::min(action_.n_orders, c.max_orders); ++slot) {
            const auto order = action_.orders[slot]; if (order.item != product) continue;
            if (order.op == M_BUY_PRODUCT) requested_buys += std::clamp(order.n, 0, c.shed_capacity);
            if (order.op == M_SELL) requested_sales += std::clamp(order.n, 0, c.shed_capacity);
        }
        // A rival can empty at most one full shed per order. This bound also
        // covers buy/sell cycles in the two input products.
        const int rival_sales_bound = c.shed_capacity * ((product == WHEAT || product == FERTILIZER) ? c.max_orders : 1);
        const int highest_inventory = pending_.market.inventory[product] + requested_sales + rival_sales_bound;
        const int consumption = demand(pending_, c, product, pending_step_);
        // Finished products cannot be bought. Their inventory only increases
        // during trading, so the observed final trading inventory is an exact
        // upper bound. Do not invent rival sales by averaging a loose bound
        // when our known sale could not have reached the price floor.
        const bool finished = product >= CARROT && product <= WOOL;
        const bool above_floor = market_price(product, highest_inventory) > 1 ||
            (finished && market_price(product, o.market.inventory[product] + consumption) > 1);
        int own_low = -requested_buys, own_high = requested_sales;
        if (fill_known) {
            const int maximum_buys = std::min(requested_buys, requested_sales - net);
            own_low = -std::max(0, maximum_buys);
            own_high = net;
            if (!requested_sales || above_floor) own_low = own_high = net;
            if (product != WHEAT && product != FERTILIZER && pending_.market.prices[product] == 1)
                own_low = own_high = 0;
        }
        const int total_effect = o.market.inventory[product] - pending_.market.inventory[product] + consumption;
        sample.lower[product] = total_effect - own_high;
        sample.upper[product] = total_effect - own_low;
        sample.identifiable[product] = fill_known && own_low == own_high;
    }
    pending_step_ = -1;
    return true;
}
}
