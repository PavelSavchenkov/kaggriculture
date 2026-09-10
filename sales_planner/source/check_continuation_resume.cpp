#include "case.hpp"
#include "continuation.hpp"
#include <chrono>

using namespace sales_planner;

bool same_account(const Account& a, const Account& b) {
    return a.cash == b.cash && a.stock == b.stock && a.seeds == b.seeds && a.total == b.total &&
        a.units == b.units && a.hires == b.hires && a.quadrants == b.quadrants;
}

bool same_resources(const Resources& a, const Resources& b) {
    if (a.buffers != b.buffers || a.key_count != b.key_count || a.missing != b.missing ||
        a.discarded != b.discarded) return false;
    for (int unit = 0; unit < kag::MAX_UNITS; ++unit)
        for (int k = 0; k < a.key_count[unit]; ++k)
            if (a.keys[unit][k] != b.keys[unit][k]) return false;
    return true;
}

int main(int argc, char** argv) {
    require(argc > 1, "usage: check_continuation_resume case.calendar [...]");
    for (int file = 1; file < argc; ++file) {
        auto c = read_case(argv[file]); certify_commitments(c);
        const auto begin = std::chrono::steady_clock::now();
        int splits = 0, checked_prefixes = 0, feasible_whole = 0;
        std::vector<int> cuts{1, 4, 5, 225, 226, 227, terminal_turn - 1};
        for (int t = 24; t < terminal_turn; t += 24)
            for (int delta : {-1, 0, 1}) cuts.push_back(t + delta);
        std::sort(cuts.begin(), cuts.end()); cuts.erase(std::unique(cuts.begin(), cuts.end()), cuts.end());
        for (int seat = 0; seat < 2; ++seat) {
            std::vector<CalendarTurn> own, rival;
            std::vector<Orders> orders, rival_orders;
            for (const auto& t : c.turns) {
                own.push_back(t.calendar[seat]); rival.push_back(t.calendar[seat ^ 1]);
                orders.push_back(t.original_orders[seat]); rival_orders.push_back(t.original_orders[seat ^ 1]);
            }
            PlannerObservation start;
            start.own = c.initial.financial.accounts[seat]; start.resources = c.initial.resources[seat];
            start.inventory = c.initial.financial.inventory;
            start.shops = c.turns[0].shops; start.n_shops = c.turns[0].n_shops;
            start.rival_cash = c.initial.financial.accounts[seat ^ 1].cash;
            const auto rules = rules_for(c.config);
            apply(start.own, start.resources, own[0].before_market, rules.capacity);
            MarketForecast world;
            world.rival = c.initial.financial.accounts[seat ^ 1]; world.rival_resources = c.initial.resources[seat ^ 1];
            world.rival_calendar = rival; world.rival_orders = rival_orders;
            for (int t = 1; t < int(c.turns.size()); ++t)
                for (int k = c.turns[t - 1].n_shops; k < c.turns[t].n_shops; ++k)
                    world.arrivals[world.n_arrivals++] = {t, c.turns[t].shops[k]};
            for (bool compact : {false, true}) {
                const auto whole = evaluate_continuation(start, own, orders, world, rules, nullptr, compact);
                feasible_whole += whole.feasible();
                require(whole.end_turn == terminal_turn, "wrong whole period end");
                if (!compact) {
                    require(whole.inventory == c.turns.back().expected.financial.inventory, "whole source market mismatch");
                    for (int side = 0; side < 2; ++side)
                        require(same_account(whole.accounts[side], c.turns.back().expected.financial.accounts[seat ^ side]),
                                "whole source account mismatch");
                }
                for (const int cut : cuts) {
                    const auto prefix = evaluate_continuation(start, std::span<const CalendarTurn>(own).first(cut),
                        std::span<const Orders>(orders).first(cut), world, rules, nullptr, compact);
                    require(prefix.end_turn == cut, "wrong prefix period end");
                    require(prefix.shops == c.turns[cut - 1].shops && prefix.n_shops == c.turns[cut - 1].n_shops,
                            "prefix boundary shop phase mismatch");
                    if (!compact) {
                        require(prefix.inventory == c.turns[cut - 1].expected.financial.inventory, "prefix source market mismatch");
                        for (int side = 0; side < 2; ++side)
                            require(same_account(prefix.accounts[side], c.turns[cut - 1].expected.financial.accounts[seat ^ side]),
                                    "prefix source account mismatch");
                        ++checked_prefixes;
                    }
                    PlannerObservation resumed{cut, prefix.accounts[0], prefix.resources[0], prefix.inventory,
                        prefix.shops, prefix.n_shops, prefix.accounts[1].cash};
                    // The preceding result is before this turn's arrivals and own work.
                    for (int k = 0; k < world.n_arrivals; ++k)
                        if (world.arrivals[k].turn == cut) resumed.shops[resumed.n_shops++] = world.arrivals[k].type;
                    apply(resumed.own, resumed.resources, own[cut].before_market, rules.capacity);
                    auto continuation = world;
                    continuation.rival = prefix.accounts[1]; continuation.rival_resources = prefix.resources[1];
                    const auto suffix = evaluate_continuation(resumed, own, orders, continuation, rules, nullptr, compact);
                    require(prefix.evaluated_turns + suffix.evaluated_turns == whole.evaluated_turns, "split turn count mismatch");
                    require(suffix.end_turn == whole.end_turn && suffix.inventory == whole.inventory &&
                        suffix.shops == whole.shops && suffix.n_shops == whole.n_shops, "split ending market mismatch");
                    for (int side = 0; side < 2; ++side) {
                        require(same_account(suffix.accounts[side], whole.accounts[side]), "split ending account mismatch");
                        require(same_resources(suffix.resources[side], whole.resources[side]), "split ending resources mismatch");
                        require(prefix.commitment_errors[side] + suffix.commitment_errors[side] == whole.commitment_errors[side],
                                "split commitment count mismatch");
                    }
                    ++splits;
                }
            }
        }
        std::printf("{\"episode\":%llu,\"splits\":%d,\"checked_prefixes\":%d,\"feasible_whole\":%d,\"seconds\":%.6f}\n",
            (unsigned long long)c.episode, splits, checked_prefixes, feasible_whole,
            std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count());
    }
}
