// Local-LB evaluation boundary (no strategy): a Kaggle observation flattened by
// scripts/lb_play.py becomes an AgentObservation for the bc_opus agent, and its
// action is returned as integers. Field order follows fast_game_engine/export_trace.py
// (tile_values, enc_unit, enc_order); layout adapted from the Sep 23 BC bridge.
#include "agent/bc_opus/source/agent.hpp"
#ifdef BRIDGE_DC11  // libdc11_lb_bridge.so: the dc11 day compiler (dc11/VENDORED.md) behind the same interface
#include "agent/bc_overhaul/source/agent.hpp"
#endif
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <span>

namespace {
using namespace kag;

void require(bool value) {
    if (!value) {
        std::fprintf(stderr, "invalid Local-LB evaluation buffer\n");
        std::abort();
    }
}

struct Reader {
    std::span<const double> values;
    size_t index = 0;
    double next() {
        require(index < values.size());
        return values[index++];
    }
    int integer() {
        const double value = next();
        require(std::isfinite(value) && value == std::trunc(value));
        return int(value);
    }
    void end() { require(index == values.size()); }
};

agent::AgentConfig parse_config(const double* values, int count) {
    Reader r{{values, size_t(count)}};
    agent::AgentConfig c;
    c.episode_steps = r.integer();
    c.board_size = r.integer();
    c.starting_money = r.integer();
    c.max_orders = r.integer();
    c.turns_per_day = r.integer();
    c.shed_capacity = r.integer();
    c.weed_chance = r.next();
    c.shop_unlock_interval = r.integer();
    c.shop_sell_interval = r.integer();
    c.center_sell_interval = r.integer();
    c.hire_mult = r.integer();
    r.end();
    return c;
}

// An optional trailing value is the remaining overage time in seconds.
agent::AgentObservation parse_observation(const double* values, int count, double* time_left) {
    Reader r{{values, size_t(count)}};
    agent::AgentObservation o;
    const int player = r.integer();
    require(player == 0 || player == 1);
    o.player = uint8_t(player);
    o.step = r.integer();
    o.day = r.integer();
    o.hour = r.integer();
    for (auto& f : o.farms) {
        f.money = r.next();
        f.n_units = r.integer();
        f.n_quadrants = r.integer();
        f.hires_today = r.integer();
        require(f.n_units >= 1 && f.n_units <= MAX_UNITS && f.n_quadrants >= 1 && f.n_quadrants <= 4);
        for (int u = 0; u < f.n_units; ++u) {
            f.pos_x[u] = int8_t(r.integer());
            f.pos_y[u] = int8_t(r.integer());
        }
        for (auto& row : f.tiles)
            for (auto& t : row) {
                const int kind = r.integer();
                require(kind >= T_EMPTY && kind <= T_PLANT);
                t.kind = TileKind(kind);
                const int what = r.integer(), animal = r.integer();
                const int watered = r.integer(), fed = r.integer(), cared = r.integer(), fertilizer = r.integer();
                const int dry = r.integer(), held = r.integer(), bank = r.integer(), planted = r.integer();
                const int expiry = r.integer(), fertilized = r.integer();
                if (kind == T_EMPTY || kind == T_LOCKED) continue;
                t.what = uint8_t(what);
                t.has_animal = animal;
                t.watered_today = watered;
                t.fed_today = fed;
                t.cared_today = cared;
                t.fertilizer_available = fertilizer;
                t.consecutive_dry = int8_t(dry);
                t.yield_units = int8_t(held);
                t.pending_care_bonus = int8_t(bank);
                t.planted_day = int16_t(planted);
                t.max_lifespan_step = expiry;
                t.fertilized_until_day = int16_t(fertilized);
            }
    }
    for (int p = 0; p < N_ITEMS; ++p) {
        o.own.shed[p] = Count(r.integer());
        o.own.shed_total += o.own.shed[p];
    }
    for (int p = 0; p < N_CROPS; ++p) o.own.seeds[p] = Count(r.integer());
    for (int u = 0; u < o.self().n_units; ++u) {
        const int keys = r.integer();
        require(keys >= 0 && keys <= N_ITEMS);
        o.own.inv_nkeys[u] = uint8_t(keys);
        for (int k = 0; k < keys; ++k) {
            const int item = r.integer(), quantity = r.integer();
            require(item >= 0 && item < N_ITEMS && quantity > 0);
            o.own.inv_keys[u][k] = uint8_t(item);
            o.own.inv[u][item] = Count(quantity);
        }
    }
    for (int p = 0; p < N_PRODUCTS; ++p) o.market.inventory[p] = r.integer();
    for (int p = 0; p < N_PRODUCTS; ++p) o.market.prices[p] = r.integer();
    o.n_shops = r.integer();
    require(o.n_shops >= 0 && o.n_shops <= MAX_SHOP_INSTANCES);
    for (int s = 0; s < o.n_shops; ++s) {
        const int shop = r.integer();
        require(shop >= 0 && shop < N_SHOPS);
        o.shops[s] = uint8_t(shop);
    }
    if (r.index < r.values.size()) *time_left = r.next();
    r.end();
    return o;
}

struct Context {
#ifdef BRIDGE_DC11
    kag::agents::bc_overhaul::Agent agent;
#else
    kag::agents::bc_opus::Agent agent;
#endif
    std::FILE* dump = nullptr;  // OPUS_DUMP: every input buffer, for tools/lb_replay
};

void write_buffer(std::FILE* f, const double* values, int count) {
    if (!f) return;
    std::fwrite(&count, sizeof count, 1, f);
    std::fwrite(values, sizeof(double), size_t(count), f);
    std::fflush(f);  // pool workers exit without closing it
}
}

extern "C" {
void* opus_new(const double* config, int count, int seat) {
    auto* c = new Context;
#ifdef BRIDGE_DC11
    if (const char* path = std::getenv("BC_OPUS_MODEL"); path && *path) c->agent.model_path = path;  // main.py and lb_play.py set it
    c->agent.options_text = "-";  // the gated defaults, unless <model>.dc11 holds dc11 options (e.g. "timing=0.5")
    if (!c->agent.model_path.empty())
        if (std::FILE* file = std::fopen((c->agent.model_path + ".dc11").c_str(), "r")) {
            std::string text;  // step 67: the whole first line (a 256-byte buffer cut longer option lines mid-key -> parser abort)
            for (char chunk[512]; std::fgets(chunk, sizeof chunk, file);) {
                text += chunk;
                if (text.back() == '\n') break;
            }
            if (!text.empty()) c->agent.options_text = text;
            std::fclose(file);
        }
#endif
    agent::AgentInit init;
    init.config = parse_config(config, count);
    init.player = uint8_t(seat);
    c->agent.reset(init);
    if (const char* path = std::getenv("OPUS_DUMP"); path && *path) {
        c->dump = std::fopen(path, "wb");
        require(c->dump);
        std::fwrite(&seat, sizeof seat, 1, c->dump);
        write_buffer(c->dump, config, count);
    }
    return c;
}

void opus_delete(void* pointer) {
    auto* c = static_cast<Context*>(pointer);
    if (c->dump) std::fclose(c->dump);
    delete c;
}

// Writes n_units, n_orders, then (op, arg, n) per unit and per order; returns the count.
int opus_act(void* pointer, const double* fields, int count, int32_t* out, int capacity) {
    auto& c = *static_cast<Context*>(pointer);
    write_buffer(c.dump, fields, count);
    double time_left = 1e9;
    const auto o = parse_observation(fields, count, &time_left);
    agent::DecisionBudget budget;
    budget.max_expansions = 256;
#ifdef BRIDGE_DC11
    // dc11 caps its work by evaluations; past the soft deadline it starts no further retries.
    if (time_left < 1e8)
        budget.soft_deadline = agent::DecisionBudget::Clock::now() +
                               std::chrono::milliseconds(int64_t(1000 * (0.9 + std::max(0.0, time_left) / std::max(1, dc10::LAST_DAY + 1 - o.day))));
#else
    c.agent.set_time_left(time_left);
#endif
    Action a;
    c.agent.act(o, budget, a);
    require(capacity >= 2 + 3 * (a.n_units + a.n_orders));
    int k = 0;
    out[k++] = a.n_units;
    out[k++] = a.n_orders;
    for (int u = 0; u < a.n_units; ++u) out[k++] = a.units[u].op, out[k++] = a.units[u].arg, out[k++] = a.units[u].n;
    for (int s = 0; s < a.n_orders; ++s) out[k++] = a.orders[s].op, out[k++] = a.orders[s].item, out[k++] = a.orders[s].n;
    return k;
}

// Summary of the latest dawn: intent, compile status, fallback and reason (truncated).
int opus_last_day(void* pointer, char* out, int capacity) {
    auto& c = *static_cast<Context*>(pointer);
    if (c.agent.reports().empty() || capacity <= 0) return 0;
    const auto& r = c.agent.reports().back();
#ifdef BRIDGE_DC11
    const std::string text = "day " + std::to_string(r.day) + " status " + std::to_string(r.status) + " fallback " +
                             std::to_string(r.fallback) + " hires " + std::to_string(r.hires) + " ms " + std::to_string(int(r.compile_ms)) +
                             " | " + r.reason.substr(0, 300);
#else
    const std::string text = "day " + std::to_string(r.day) + " status " + std::to_string(r.status) + " fallback " +
                             std::to_string(r.fallback) + " hires " + std::to_string(r.hires) + " returns " + std::to_string(r.return_percent) + " ms " + std::to_string(int(r.compile_ms)) + " | " +
                             kag::agents::bc_opus::summarize(c.agent.last_intent(), c.agent.last_schema()) + " | " +
                             r.reason.substr(0, 300);
#endif
    const int n = std::min<int>(capacity - 1, int(text.size()));
    std::copy_n(text.data(), n, out);
    out[n] = 0;
    return n;
}

// Per-game compile statistics: uncompiled days, fallback days, invalid intents, max compile ms.
void opus_stats(void* pointer, double* out) {
    auto& c = *static_cast<Context*>(pointer);
    out[0] = out[1] = out[2] = out[3] = 0;
    for (const auto& r : c.agent.reports()) {
        out[0] += r.status != 0;
        out[1] += r.status == 0 && r.fallback != 0;
#ifndef BRIDGE_DC11
        out[2] += !r.invalid.empty();
#endif
        out[3] = std::max(out[3], r.compile_ms);
    }
}
}
