#include "trading.hpp"
#include <algorithm>
#include <cstdlib>

namespace kag::day_compiler {
int wheat_inventory_bound(int inventory,int quantity,int capacity) {
    // Wheat's logarithmic curve never reaches $1 in the engine's inventory
    // range. Every sale restores one market unit, so even repeated buy/sell
    // cycles change inventory by at most the rival's shed capacity at any prefix.
    // This argument does not apply to fertilizer, whose $1 sales discard supply.
    return inventory+(quantity>0?capacity:quantity<0?-capacity:0);
}
Transaction transact(int product, int inventory, int own, int rival) {
    Transaction result{inventory};
    const int own_count = std::abs(own), rival_count = std::abs(rival);
    for (int unit = 0; unit < std::max(own_count, rival_count); ++unit) {
        const int own_price = unit < own_count ? market_price(product, result.inventory - (own < 0)) : 0;
        const int rival_price = unit < rival_count ? market_price(product, result.inventory - (rival < 0)) : 0;
        if (unit < own_count) {
            result.own_cash += own > 0 ? own_price : -own_price;
            result.inventory += own > 0 ? own_price > 1 : -1;
        }
        if (unit < rival_count) {
            result.rival_cash += rival > 0 ? rival_price : -rival_price;
            result.inventory += rival > 0 ? rival_price > 1 : -1;
        }
    }
    return result;
}
OrderLedger::OrderLedger(const Observation& observation, const Farm& farm, const Configuration& config) : config_(config) {
    cash = farm.money; total = farm.shed_total; hires = farm.hires_today; quadrants = farm.n_quadrants;
    std::copy_n(farm.shed, N_ITEMS, stock); std::copy_n(farm.seeds, N_CROPS, seeds);
    std::copy_n(observation.market.inventory, N_PRODUCTS, inventory);
}
bool OrderLedger::append(Order order) {
    if (order.op == M_NONE) return true;
    if (orders.n_orders == config_.max_orders || orders.n_orders == 10) return false;
    auto next = *this;
    if (order.op == M_HIRE) {
        const int price = config_.hire_mult * fib(hires);
        if (hires + 1 >= MAX_UNITS || cash < price) return false;
        next.cash -= price; ++next.hires;
    } else if (order.op == M_BUY_LAND) {
        if (quadrants >= 4 || cash < LAND_PRICES[quadrants - 1]) return false;
        next.cash -= LAND_PRICES[quadrants - 1]; ++next.quadrants;
    } else {
        const int product = order.item;
        if (order.n <= 0 || product >= N_ITEMS) return false;
        for (int quantity = 0; quantity < order.n; ++quantity) {
            if (order.op == M_SELL && is_product(product)) {
                if (!next.stock[product]) return false;
                const int price = market_price(product, next.inventory[product]);
                next.cash += price; next.inventory[product] += price > 1;
                --next.stock[product]; --next.total;
            } else if (order.op == M_BUY_PRODUCT && (product == WHEAT || product == FERTILIZER)) {
                const int price = market_price(product, next.inventory[product] - 1);
                if (next.cash < price || next.total == config_.shed_capacity) return false;
                next.cash -= price; --next.inventory[product]; ++next.stock[product]; ++next.total;
            } else if (order.op == M_BUY_SEED && is_crop(product)) {
                const int price = CROPS[product].seed;
                if (next.cash < price) return false;
                next.cash -= price; ++next.seeds[product];
            } else if (order.op == M_BUY_ANIMAL && is_animal(product)) {
                const int price = ANIMALS[product - GOOSE].cost;
                if (next.cash < price || next.total == config_.shed_capacity) return false;
                next.cash -= price; ++next.stock[product]; ++next.total;
            } else return false;
        }
    }
    next.orders.orders[next.orders.n_orders++] = order; next.orders.finalize();
    *this = next;
    return true;
}
bool OrderLedger::extend_finished_sale(int product,int quantity) {
    if(product<CARROT || product>WOOL || quantity<=0 || quantity>stock[product]) return false;
    for(int k=0;k<orders.n_orders;++k) if(orders.orders[k].op==M_SELL && orders.orders[k].item==product) {
        const auto trade=transact(product,inventory[product],quantity,0);
        cash+=trade.own_cash; inventory[product]=trade.inventory;
        stock[product]-=quantity; total-=quantity; orders.orders[k].n+=quantity;
        return true;
    }
    return false;
}
bool finished_sales_first(const Observation& o,const Farm& workers,Action& action,const Configuration& config) {
    OrderLedger ledger(o,workers,config);
    for(bool sales:{true,false}) for(int k=0;k<action.n_orders;++k) {
        const auto order=action.orders[k];
        const bool finished=order.op==M_SELL && order.item>=CARROT && order.item<=WOOL;
        if(finished==sales && !ledger.append(order)) return false;
    }
    action.n_orders=ledger.orders.n_orders;
    std::copy_n(ledger.orders.orders,action.n_orders,action.orders); action.finalize();
    return true;
}
bool fertilizer_sales_after_products(const Observation& o,const Farm& workers,Action& action,const Configuration& config) {
    int prefix=0;
    for(;prefix<action.n_orders;++prefix) {
        const auto x=action.orders[prefix];
        if(x.op!=M_NONE && !(x.op==M_SELL && x.item>=CARROT && x.item<=WOOL))break;
    }
    bool early[10]{},bought=false,any=false;
    for(int k=prefix;k<action.n_orders;++k) {
        const auto x=action.orders[k];
        bought|=x.op==M_BUY_PRODUCT && x.item==FERTILIZER;
        early[k]=!bought && x.op==M_SELL && x.item==FERTILIZER;
        any|=early[k];
    }
    if(!any)return true;
    OrderLedger ledger(o,workers,config);
    for(int k=0;k<prefix;++k)if(!ledger.append(action.orders[k]))return false;
    for(bool selected:{true,false})for(int k=prefix;k<action.n_orders;++k)
        if(early[k]==selected && !ledger.append(action.orders[k]))return false;
    action.n_orders=ledger.orders.n_orders;
    std::copy_n(ledger.orders.orders,action.n_orders,action.orders); action.finalize();
    return true;
}
}
