#pragma once
#include "calendar.hpp"

namespace sales_planner {
struct SeedBudgetStats { int orders_changed = 0, units_removed = 0; };

// Exact remaining uses from the supplied plan, plus its explicit ending seed
// requirement. This does not predict new planting decisions or their biology.
class SeedBudget {
    std::vector<std::array<int, kag::N_CROPS>> after_work_;
public:
    SeedBudget(std::span<const CalendarTurn> calendar,
               std::array<int, kag::N_CROPS> ending_seeds,
               std::span<const Orders> planned_restock = {}) : after_work_(calendar.size()) {
        if (!planned_restock.empty() && planned_restock.size() != calendar.size()) std::abort();
        for (int n : ending_seeds) if (n < 0) std::abort();
        auto remaining = ending_seeds;
        auto add_uses = [&](std::span<const ResourceEvent> events) {
            for (const auto e : events) if (e.flow == Flow::use_seed) {
                if (e.item >= kag::N_CROPS || e.quantity < 0) std::abort();
                remaining[e.item] += e.quantity;
            }
        };
        for (int t = int(calendar.size()) - 1; t >= 0; --t) {
            add_uses(calendar[t].after_market);
            after_work_[t] = remaining;
            // Optional alternative: leave later needs to supplied later buys.
            // Their funding is conditional, so full-game verification remains
            // necessary. This is not a proof that a future request will fill.
            if (!planned_restock.empty()) for (int k = 0; k < planned_restock[t].count; ++k) {
                const auto o = planned_restock[t].values[k];
                if (o.op == kag::M_BUY_SEED && o.n > 0) {
                    if (o.item >= kag::N_CROPS) std::abort();
                    remaining[o.item] = std::max(0, remaining[o.item] - o.n);
                }
            }
            add_uses(calendar[t].before_market);
        }
    }

    // Preserve dates and positions. Reduce only quantities above everything
    // this fixed plan could still use or require at its end. Seed prices are
    // fixed and seed storage is separate; seeds cannot be sold or transferred.
    Orders cap(int turn, const Account& after_work, Orders orders, SeedBudgetStats& stats) const {
        if (turn < 0 || turn >= int(after_work_.size())) std::abort();
        auto needed = after_work_[turn];
        for (int item = 0; item < kag::N_CROPS; ++item)
            needed[item] = std::max(0, needed[item] - after_work.seeds[item]);
        for (int k = 0; k < orders.count; ++k) {
            auto& o = orders.values[k];
            if (o.op != kag::M_BUY_SEED || o.n <= 0) continue;
            if (o.item >= kag::N_CROPS) std::abort();
            const int n = std::min(o.n, needed[o.item]);
            needed[o.item] -= n;
            if (n == o.n) continue;
            ++stats.orders_changed; stats.units_removed += o.n - n;
            if (n) o.n = n;
            else o = {};
        }
        return orders;
    }
};
}
