#include "planning_context.hpp"
#include <iostream>

int main() {
    try {
        int checks = 0;
        day_solver::DayProblem p;
        p.market_plan.push_back({0, 0, kag::M_BUY_SEED, kag::WHEAT, 1, 0});
        for (int hours : {23, 24}) for (int first : {0, 3, 8, 17}) for (int rate : {2, 10}) {
            std::vector<std::pair<int, int>> slots;
            for (int h = first; h < hours - 1 && slots.size() < 39; ++h)
                for (int slot = 0; slot < rate && slots.size() < 39; ++slot) if (h || slot) slots.emplace_back(h, slot);
            const auto menu = labor::planning_menu(p, hours, slots);
            for (int workers = 1; workers <= menu.size + 1; ++workers) for (int radius = 0; radius <= 8; ++radius)
                for (int through = 0; through < hours; ++through) {
                    int expected = 0;
                    for (int h = 0; h <= through; ++h) for (int u = 0; u < workers; ++u)
                        expected += h >= (u ? slots[u - 1].first + 1 : 0) + radius;
                    if (labor::planning_capacity(menu, workers, through, radius) != expected) throw std::runtime_error("calendar capacity mismatch");
                    ++checks;
                }
        }
        auto rejected = [&](std::vector<std::pair<int, int>> slots) {
            bool failed = false;
            try { labor::planning_menu(p, 24, slots); } catch (const std::runtime_error&) { failed = true; }
            if (!failed) throw std::runtime_error("invalid menu accepted");
            ++checks;
        };
        rejected({{0, 0}}); rejected({{0, 1}, {0, 1}}); rejected({{1, 0}, {0, 1}});
        rejected({{23, 0}}); rejected({{0, 10}}); rejected({{-1, 0}});
        const auto empty = labor::planning_menu(p, 23, {});
        if (empty.size || labor::planning_capacity(empty, 1) != 23) throw std::runtime_error("empty calendar mishandled");
        const std::array<std::pair<int, int>, 2> fixed{{{8, 0}, {14, 0}}};
        const std::array<std::pair<int, int>, 3> optional{{{0, 1}, {1, 0}, {2, 0}}};
        const auto mixed = labor::fixed_planning_menu(p, 24, fixed, optional);
        if (mixed.minimum_workers != 3 || mixed.size != 5) throw std::runtime_error("fixed workforce lost");
        for (int k = 3; k <= 6; ++k) {
            int expected = 24 + 15 + 9;
            for (int i = 0; i < k - 3; ++i) expected += 23 - optional[i].first;
            if (labor::planning_capacity(mixed, k) != expected) throw std::runtime_error("early optional hire displaced fixed hire");
            ++checks;
        }
        bool below_fixed_rejected = false;
        try { labor::planning_capacity(mixed, 2); } catch (const std::runtime_error&) { below_fixed_rejected = true; }
        if (!below_fixed_rejected) throw std::runtime_error("requested workforce omits fixed commitments");
        const auto context = labor::extract_context(p, mixed);
        if (context.back() != 3 || context[labor::planning_lower] < 3) throw std::runtime_error("committed workforce absent from features");
        checks += 2;
        const std::array<std::pair<int, int>, 1> last_phase{{{22, 0}}};
        const auto inactive = labor::fixed_planning_menu(p, 23, last_phase, {});
        if (inactive.minimum_workers != 2 || labor::planning_capacity(inactive, 2) != 23)
            throw std::runtime_error("inactive fixed hire lost or given action capacity");
        ++checks;
        std::cout << "Planning checks passed: " << checks + 1 << " capacities and calendar validation controls\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
