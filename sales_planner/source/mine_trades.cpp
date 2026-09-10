#include "case.hpp"
#include "baseline.hpp"
#include "compact.hpp"

using namespace sales_planner;
int main(int argc, char** argv) {
    require(argc > 1, "usage: mine_trades calendar [...]");
    for (int file = 1; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        auto state = c.initial.financial; auto resources = c.initial.resources;
        const auto rules = rules_for(c.config);
        std::array<std::vector<CalendarTurn>, 2> calendars;
        for (const auto& turn : c.turns)
            for (int p = 0; p < 2; ++p) calendars[p].push_back(turn.calendar[p]);
        std::array<Items, 2> first_available;
        for (auto& a : first_available) a.fill(-1);
        for (int t = 0; t < int(c.turns.size()); ++t) {
            const auto& turn = c.turns[t];
            state.n_shops = turn.n_shops; state.shops = turn.shops;
            for (int p = 0; p < 2; ++p)
                apply(state.accounts[p], resources[p], turn.calendar[p].before_market, rules.capacity);
            const auto before = state;
            const auto outcome = trade(state, turn.original_orders, rules);
            std::array<Items, 2> bought{}, sold{};
            std::array<Items, 2> first_sale, first_buy;
            int noops[2]{}, ineffective_sales[2]{}, zero_quantity[2]{};
            for (int p = 0; p < 2; ++p) {
                first_sale[p].fill(-1); first_buy[p].fill(-1);
                const auto& orders = turn.original_orders[p];
                for (int k = 0; k < orders.count; ++k) {
                    const auto o = orders.values[k]; const int n = outcome.accepted[p][k];
                    noops[p] += n == 0; zero_quantity[p] += o.n <= 0;
                    if (o.op == kag::M_SELL && o.item < kag::N_PRODUCTS) {
                        ineffective_sales[p] += n == 0;
                        sold[p][o.item] += n;
                        if (n && first_sale[p][o.item] < 0) first_sale[p][o.item] = k;
                    }
                    if ((o.op == kag::M_BUY_PRODUCT || o.op == kag::M_BUY_ANIMAL) && o.item < kag::N_ITEMS) {
                        bought[p][o.item] += n;
                        if (n && first_buy[p][o.item] < 0) first_buy[p][o.item] = k;
                    }
                }
            }
            for (int p = 0; p < 2; ++p) {
                const auto& account = before.accounts[p];
                PlannerObservation obs{t, account, resources[p], before.inventory, before.shops, before.n_shops, before.accounts[p ^ 1].cash};
                const auto reserve = needs(obs, calendars[p], 4);
                std::printf("{\"kind\":\"turn\",\"episode\":%llu,\"seat\":%d,\"turn\":%d,\"orders\":%d,\"ineffective\":%d,\"ineffective_sales\":%d,\"zero_quantity\":%d,\"cash\":%.0f,\"stock_total\":%d,\"receipts\":%.0f,\"spending\":%.0f}\n",
                            (unsigned long long)c.episode,p,t,turn.original_orders[p].count,noops[p],ineffective_sales[p],zero_quantity[p],account.cash,account.total,outcome.receipts[p],outcome.spending[p]);
                for (int item = 0; item < kag::N_ITEMS; ++item) {
                    if (!account.stock[item]) first_available[p][item] = -1;
                    else if (first_available[p][item] < 0) first_available[p][item] = t;
                    if (!account.stock[item] && !bought[p][item] && !sold[p][item]) continue;
                    const bool product = item < kag::N_PRODUCTS;
                    int demand = 0;
                    if (product) {
                        if (t % rules.shop_interval == 0)
                            for (int s = 0; s < before.n_shops; ++s)
                                if (kag::SHOP_MASK[before.shops[s]] & (1u << item)) demand += kag::SHOP_MULT[before.shops[s]];
                        if (t % rules.town_interval == 0 && item != kag::FERTILIZER) ++demand;
                    }
                    std::printf("{\"kind\":\"item\",\"episode\":%llu,\"seat\":%d,\"turn\":%d,\"item\":%d,\"stock\":%d,\"sold\":%d,\"bought\":%d,\"sale_slot\":%d,\"buy_slot\":%d,\"age\":%d,\"reserve4\":%d,\"capacity_left\":%d,\"price\":%d,\"inventory\":%d,\"demand_after\":%d,\"rival_ready\":%d,\"rival_sold\":%d,\"rival_bought\":%d}\n",
                                (unsigned long long)c.episode,p,t,item,account.stock[item],sold[p][item],bought[p][item],first_sale[p][item],first_buy[p][item],
                                first_available[p][item] < 0 ? -1 : t-first_available[p][item],reserve.stock[item],rules.capacity-account.total,
                                product ? kag::market_price(item,before.inventory[item]) : 0,product ? before.inventory[item] : 0,demand,
                                product ? turn.public_ready[p ^ 1][item] : 0,sold[p ^ 1][item],bought[p ^ 1][item]);
                    if (product && sold[p][item] >= account.stock[item] && !state.accounts[p].stock[item]) first_available[p][item] = -1;
                }
                double prior_receipts = 0, prior_spending = 0;
                for (int k = 0; k < turn.original_orders[p].count; ++k) {
                    const auto o = turn.original_orders[p].values[k];
                    auto prefix_state = before; auto prefix_orders = turn.original_orders;
                    for (auto& orders : prefix_orders) orders.count = std::min(orders.count,k+1);
                    const auto prefix_outcome = trade(prefix_state,prefix_orders,rules);
                    const double delta = prefix_outcome.receipts[p] - prior_receipts - prefix_outcome.spending[p] + prior_spending;
                    prior_receipts = prefix_outcome.receipts[p]; prior_spending = prefix_outcome.spending[p];
                    if (outcome.accepted[p][k])
                        std::printf("{\"kind\":\"order\",\"episode\":%llu,\"seat\":%d,\"turn\":%d,\"slot\":%d,\"op\":%d,\"item\":%d,\"requested\":%d,\"accepted\":%d,\"cash_delta\":%.0f}\n",
                                    (unsigned long long)c.episode,p,t,k,o.op,o.item,o.n,outcome.accepted[p][k],delta);
                }
            }
            consume(state,rules);
            for (int p = 0; p < 2; ++p) apply(state.accounts[p],resources[p],turn.calendar[p].after_market,rules.capacity);
            advance(state,rules);
            require(state.inventory == turn.expected.financial.inventory,"mining market mismatch");
            for (int p=0;p<2;++p) require(state.accounts[p].cash==turn.expected.financial.accounts[p].cash,"mining cash mismatch");
        }
    }
}
