#pragma once
#include "calendar.hpp"
#include <fstream>
#include <string>

namespace sales_planner {
inline void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}

struct Truth {
    MarketState financial;
    std::array<Resources, 2> resources;
};
struct CaseTurn {
    std::array<CalendarTurn, 2> calendar;
    std::array<Orders, 2> original_orders;
    std::array<Orders, 2> purchase_witness;
    std::array<kag::Action, 2> original_actions;
    Truth expected;
    std::array<uint8_t, 8> shops{};
    int n_shops = 0;
    std::array<std::array<int, kag::N_PRODUCTS>, 2> public_ready{};
};
struct Case {
    uint64_t episode = 0;
    kag::Config config;
    Truth initial;
    std::vector<CaseTurn> turns;
};

inline void read_account(std::istream& in, Account& a, Resources& r) {
    in >> a.cash >> a.total >> a.units >> a.hires >> a.quadrants;
    require(a.units > 0 && a.units <= kag::MAX_UNITS, "invalid account unit count");
    for (auto& n : a.stock) in >> n;
    for (auto& n : a.seeds) in >> n;
    for (int b = 0; b < a.units; ++b) {
        int count = 0; in >> count;
        require(count >= 0 && count <= kag::N_ITEMS, "invalid resource key count");
        for (int k = 0; k < count; ++k) {
            int item = 0, n = 0; in >> item >> n;
            require(item >= 0 && item < kag::N_ITEMS && n > 0, "invalid resource item");
            r.add(b, item, n);
        }
    }
}

inline Case read_case(const std::string& path) {
    std::ifstream in(path); require(bool(in), "cannot open calendar");
    Case c; std::string version; int length = 0;
    in >> version >> c.episode >> length >> c.config.seed;
    require(version == "SPCAL1" && length == terminal_turn, "unsupported calendar version/horizon");
    auto& cfg = c.config;
    in >> cfg.episode_steps >> cfg.board_size >> cfg.starting_money >> cfg.max_orders
       >> cfg.turns_per_day >> cfg.shed_capacity >> cfg.weed_chance >> cfg.shop_unlock_interval
       >> cfg.shop_sell_interval >> cfg.center_sell_interval >> cfg.hire_mult;
    for (auto& n : c.initial.financial.inventory) in >> n;
    for (int p = 0; p < 2; ++p) read_account(in, c.initial.financial.accounts[p], c.initial.resources[p]);
    c.turns.resize(length);
    for (int t = 0; t < length; ++t) {
        auto& turn = c.turns[t];
        in >> turn.n_shops;
        require(turn.n_shops >= 0 && turn.n_shops <= 8, "invalid shops");
        for (int i = 0; i < 8 && i < turn.n_shops; ++i) {
            int shop = 0; in >> shop; require(shop >= 0 && shop < 8, "invalid shop id");
            turn.shops[i] = shop;
        }
        for (int p = 0; p < 2; ++p) {
            auto& plan = turn.calendar[p];
            for (auto* events : {&plan.before_market, &plan.after_market}) {
                int count = 0; in >> count;
                require(count >= 0 && count <= 1000, "invalid event count");
                events->resize(count);
                for (auto& e : *events) {
                    int flow = 0, item = 0, buffer = 0;
                    in >> flow >> item >> buffer >> e.quantity;
                    require(flow >= 0 && flow <= 6 && item >= 0 && item < kag::N_ITEMS &&
                            buffer >= 0 && buffer < kag::MAX_UNITS && e.quantity >= 0, "invalid event");
                    e.flow = Flow(flow); e.item = item; e.buffer = buffer;
                }
            }
            auto& orders = turn.original_orders[p]; auto& action = turn.original_actions[p];
            in >> orders.count;
            require(orders.count >= 0 && orders.count <= cfg.max_orders, "invalid orders");
            action.n_orders = plan.commitments.count = orders.count;
            for (int slot = 0; slot < orders.count; ++slot) {
                int op = 0, item = 0, n = 0; in >> op >> item >> n;
                require(op >= 0 && op <= kag::M_SELL && item >= 0 && item <= 255, "invalid order");
                orders.values[slot] = action.orders[slot] = {uint8_t(op), uint8_t(item), n};
                if (op == kag::M_HIRE || op == kag::M_BUY_LAND) plan.commitments.values[slot] = orders.values[slot];
            }
            in >> action.n_units;
            require(action.n_units >= 1 && action.n_units <= kag::MAX_UNITS, "invalid action units");
            for (int u = 0; u < action.n_units; ++u) {
                int op = 0, item = 0, n = 0; in >> op >> item >> n;
                action.units[u] = {uint8_t(op), uint8_t(item), n};
            }
            action.finalize();
        }
        turn.expected.financial.turn = t + 1;
        for (int p = 0; p < 2; ++p)
            read_account(in, turn.expected.financial.accounts[p], turn.expected.resources[p]);
        for (auto& n : turn.expected.financial.inventory) in >> n;
        require(bool(in), "truncated calendar");
    }
    std::string extra; require(!(in >> extra), "trailing calendar content");
    return c;
}

inline MarketRules rules_for(const kag::Config& c) {
    return {c.shed_capacity, c.max_orders, c.hire_mult, c.turns_per_day,
            c.shop_sell_interval, c.center_sell_interval};
}

// Requirements are successful source commitments, not failed requests that
// never supplied a worker or land to the source production plan.
inline void certify_commitments(Case& c) {
    auto s = c.initial.financial; auto r = c.initial.resources;
    const auto rules = rules_for(c.config);
    kag::Sim source(c.config);
    for (auto& turn : c.turns) {
        for (int p = 0; p < 2; ++p)
            for (const auto& row : source.st.farms[p].tiles)
                for (const auto& tile : row) {
                    if (tile.has_animal) turn.public_ready[p][kag::ANIMALS[tile.what - kag::GOOSE].product] += tile.yield_units;
                    else if (tile.kind == kag::T_PLANT && source.st.day - tile.planted_day >= kag::CROPS[tile.what].first_yield_day)
                        turn.public_ready[p][tile.what] += tile.yield_units;
                }
        s.n_shops = turn.n_shops; s.shops = turn.shops;
        for (int p = 0; p < 2; ++p) apply(s.accounts[p], r[p], turn.calendar[p].before_market, rules.capacity);
        const auto outcome = trade(s, turn.original_orders, rules);
        for (int p = 0; p < 2; ++p) {
            // An optional funded purchase witness for a narrower sales-only
            // contract. Keep sale requests and every order position unchanged.
            auto& witness = turn.purchase_witness[p];
            witness = turn.original_orders[p];
            for (int slot = 0; slot < witness.count; ++slot) {
                auto& o = witness.values[slot];
                if (o.op < kag::M_HIRE || o.op > kag::M_BUY_ANIMAL) continue;
                const int n = outcome.accepted[p][slot];
                if (n) o.n = n;
                else o = {};
            }
            auto& commitments = turn.calendar[p].commitments;
            for (int slot = 0; slot < commitments.count; ++slot)
                if (commitments.values[slot].op && !outcome.accepted[p][slot]) commitments.values[slot] = {};
        }
        consume(s, rules);
        for (int p = 0; p < 2; ++p) apply(s.accounts[p], r[p], turn.calendar[p].after_market, rules.capacity);
        advance(s, rules);
        require(s.inventory == turn.expected.financial.inventory, "certification market mismatch");
        for (int p = 0; p < 2; ++p) {
            require(s.accounts[p].cash == turn.expected.financial.accounts[p].cash, "certification cash mismatch");
            require(s.accounts[p].stock == turn.expected.financial.accounts[p].stock, "certification stock mismatch");
            for (int n : r[p].missing) require(n == 0, "source resource requirement failed");
        }
        source.step(turn.original_actions[0], turn.original_actions[1]);
    }
}
}
