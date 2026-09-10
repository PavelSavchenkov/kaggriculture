#pragma once
#include "market.hpp"
#include <vector>

namespace sales_planner {
// Buffers name quantities outside the shed. They carry no tile or route data.
// A producer can use worker inventories as buffers, but finance only sees flows.
enum class Flow : uint8_t { produce, use, withdraw, deposit, drop, use_seed, drop_all };
struct ResourceEvent {
    Flow flow{};
    uint8_t item = 0, buffer = 0;
    int quantity = 0;
};
struct Resources {
    std::array<Items, kag::MAX_UNITS> buffers{};
    std::array<std::array<uint8_t, kag::N_ITEMS>, kag::MAX_UNITS> keys{};
    std::array<int, kag::MAX_UNITS> key_count{};
    Items missing{}, discarded{};
    void add(int b, int item, int n) {
        if (n <= 0) return;
        if (buffers[b][item] == 0) keys[b][key_count[b]++] = item;
        buffers[b][item] += n;
    }
    void take(int b, int item, int n) {
        if (n <= 0) return;
        buffers[b][item] -= n;
        if (buffers[b][item] != 0) return;
        int k = 0;
        while (k < key_count[b] && keys[b][k] != item) ++k;
        if (k == key_count[b]) std::abort();
        for (int j = k; j + 1 < key_count[b]; ++j) keys[b][j] = keys[b][j + 1];
        --key_count[b];
    }
};

inline void apply(Account& a, Resources& r, std::span<const ResourceEvent> events,
                  int capacity = 100) {
    for (const auto e : events) {
        if (e.item >= kag::N_ITEMS || e.buffer >= kag::MAX_UNITS || e.quantity < 0) std::abort();
        auto& buffer = r.buffers[e.buffer][e.item];
        int moved = 0;
        switch (e.flow) {
        case Flow::produce: r.add(e.buffer, e.item, e.quantity); break;
        case Flow::use:
            moved = std::min(buffer, e.quantity); r.take(e.buffer, e.item, moved);
            r.missing[e.item] += e.quantity - moved; break;
        case Flow::withdraw:
            moved = std::min(a.stock[e.item], e.quantity);
            a.stock[e.item] -= moved; a.total -= moved; r.add(e.buffer, e.item, moved);
            r.missing[e.item] += e.quantity - moved; break;
        case Flow::deposit:
        case Flow::drop: {
            const int offered = std::min(buffer, e.quantity);
            moved = std::min(offered, capacity - a.total);
            a.stock[e.item] += moved; a.total += moved;
            if (e.flow == Flow::drop) {
                r.discarded[e.item] += offered - moved; r.take(e.buffer, e.item, offered);
            } else r.take(e.buffer, e.item, moved);
            break;
        }
        case Flow::drop_all:
            while (r.key_count[e.buffer]) {
                const int item = r.keys[e.buffer][0], offered = r.buffers[e.buffer][item];
                moved = std::min(offered, capacity - a.total);
                a.stock[item] += moved; a.total += moved;
                r.discarded[item] += offered - moved;
                r.take(e.buffer, item, offered);
            }
            break;
        case Flow::use_seed:
            if (e.item >= kag::N_CROPS) std::abort();
            // Plant demand is checked for all same-turn requests by the producer.
            moved = std::min(a.seeds[e.item], e.quantity); a.seeds[e.item] -= moved;
            r.missing[e.item] += e.quantity - moved; break;
        }
    }
}

struct CalendarTurn {
    std::vector<ResourceEvent> before_market, after_market;
    Orders commitments; // v0: fixed hires/land; original trades kept separately.
};
struct Calendar {
    int start = 0, end = terminal_turn;
    Account starting_account;
    Resources starting_resources;
    std::vector<CalendarTurn> turns;
    Items end_stock_min{}, end_buffer_min{};
    std::array<int, kag::N_CROPS> end_seeds_min{};
    bool schedule_certified = false;
};
}
