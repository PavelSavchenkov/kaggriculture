// Sales audit: how one seat of each trace sells. Per game: units sold, revenue, average
// sale price relative to the product's base price, share of units sold in hours 0-5,
// 6-17 and 18-23, mean sellable shed stock at dawn, discarded units, final money and
// units sold per product.
// SALES_ROWS=<csv>: also one row per day, hour and product sold (units, market price before the
// step, base price) for sale-timing analyses.
// usage: sales_audit list.txt out.csv   (list line: trace seat label)
#include "source/world.hpp"
#include <iostream>
#include <sstream>
#include "tools/shop_crn.hpp"

using namespace dc10;

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: sales_audit list.txt out.csv\n";
        return 2;
    }
    std::ifstream list(argv[1]);
    std::ofstream out(argv[2]);
    std::ofstream rows;
    if (const char* path = std::getenv("SALES_ROWS")) {
        rows.open(path);
        rows << "trace,seat,label,day,hour,product,units,price,base,shed\n";
    }
    out << "trace,seat,label,units,revenue,price_vs_base,early,mid,late,dawn_stock,discarded,money,"
           "wheat,carrot,tomato,strawberry,melon,egg,milk,wool,fertilizer\n";
    for (std::string line; std::getline(list, line);) {
        std::istringstream fields(line);
        std::string trace, label;
        int seat = 0;
        if (!(fields >> trace >> seat >> label)) continue;
        const Replay replay = load_replay(trace);
        Sim sim(replay.config);
        int crn_shops = 0;  // SHOP_CRN: shops fixed so far
        double units = 0, revenue = 0, base_value = 0, by_hour[3]{}, stock = 0;
        int dawns = 0;
        for (size_t s = 0; s < replay.turns.size(); ++s) {
            const Farm& before = sim.st.farms[seat];
            const int hour = int(s % HOURS);
            if (hour == 0) {
                ++dawns;
                for (int p = 0; p < N_PRODUCTS; ++p)
                    if (p != WHEAT && p != FERTILIZER) stock += before.shed[p];
            }
            int32_t sold[N_PRODUCTS];
            std::copy_n(before.sold_units, N_PRODUCTS, sold);
            int price[N_PRODUCTS];
            for (int p = 0; p < N_PRODUCTS; ++p) price[p] = market_price(p, sim.st.market.inventory[p]);
            const int shed = before.shed_total;
            const double paid = before.sell_revenue;
            sim.step(replay.turns[s][0], replay.turns[s][1]);
            shop_crn(sim, crn_shops);
            const Farm& after = sim.st.farms[seat];
            for (int p = 0; p < N_PRODUCTS; ++p) {
                const int n = after.sold_units[p] - sold[p];
                units += n;
                base_value += double(n) * MARKET[p].base;
                by_hour[hour < 6 ? 0 : hour < 18 ? 1 : 2] += n;
                if (n && rows.is_open())
                    rows << trace << ',' << seat << ',' << label << ',' << s / HOURS << ',' << hour << ',' << p << ',' << n << ','
                         << price[p] << ',' << MARKET[p].base << ',' << shed << '\n';
            }
            revenue += after.sell_revenue - paid;
        }
        int discarded = 0;
        for (int i = 0; i < N_ITEMS; ++i) discarded += sim.st.farms[seat].discarded[i];
        out << trace << ',' << seat << ',' << label << ',' << units << ',' << revenue << ',' << (base_value ? revenue / base_value : 0)
            << ',' << by_hour[0] / std::max(1.0, units) << ',' << by_hour[1] / std::max(1.0, units) << ','
            << by_hour[2] / std::max(1.0, units) << ',' << stock / std::max(1, dawns) << ',' << discarded << ','
            << sim.st.farms[seat].money;
        for (int p = 0; p < N_PRODUCTS; ++p) out << ',' << sim.st.farms[seat].sold_units[p];
        out << '\n';
    }
    return 0;
}
