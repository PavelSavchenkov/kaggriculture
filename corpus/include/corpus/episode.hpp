#pragma once
// Read a .kagz episode straight into the fast engine.
//
// This is the seam the mining stack was missing: day_contract.hpp and the
// composition tooling consume a kag::Sim plus per-hour kag::Action values, and
// until now nothing could turn a downloaded Kaggle replay into those. The
// container stores the action stream and the engine regenerates state, so a
// load is cheap and a full re-simulation is ~0.2 ms.
//
// Header-only. Needs zlib (-lz) and a directory containing fast_game_engine/
// on the include path. The qualified path matters: day_solver vendors a
// byte-identical copy of sim.hpp, and two copies reached by different spellings
// are distinct files to #pragma once, so both would be included and every kag::
// symbol would double-define.
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <zlib.h>

#include "fast_game_engine/sim.hpp"

namespace corpus {

// Same orderings as fast_game_engine/export_trace.py and the sim.hpp enums.
inline constexpr const char* OPS[] = {
    "PASS", "NORTH", "SOUTH", "EAST", "WEST", "PICKUP", "DROP", "PLACE",
    "PLANT", "WATER", "HARVEST", "FERTILIZE", "DIG",
    "BUILD_COOP", "BUILD_PASTURE", "FEED", "COLLECT_FERTILIZER", "CARE"};
inline constexpr const char* ITEMS[] = {
    "WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL",
    "FERTILIZER", "GOOSE", "COW", "SHEEP"};
inline constexpr const char* MOPS[] = {
    "NONE", "HIRE", "BUY_LAND", "BUY_SEED", "BUY_PRODUCT", "BUY_ANIMAL", "SELL"};

constexpr int MAX_ORDERS = 16;      // kag::Action::orders capacity
constexpr uint8_t ELEM_INT = 0xFE;   // element tag: a varint integer follows
constexpr uint8_t ELEM_NULL = 0xFF;  // element tag: a JSON null

struct Episode {
    uint64_t episode_id = 0;
    uint64_t seed = 0;
    kag::Config config;
    std::string engine_version, engine_sha256;
    std::string teams[2], statuses[2];
    double rewards[2] = {0, 0};
    std::vector<uint64_t> submission_ids;
    int n_steps = 0, n_seats = 0;
    // actions[t] is the action recorded on state t, i.e. the one that drove
    // t-1 -> t. Index 0 is the framework's placeholder and is never applied.
    std::vector<std::array<kag::Action, 2>> actions;
    std::vector<uint64_t> hashes;   // canonical parity hash of every state
    uint64_t chain_hash = 0;
};

namespace detail {

struct Cursor {
    const uint8_t* p;
    const uint8_t* end;
    bool bad = false;

    bool need(size_t n) {
        if (size_t(end - p) < n) { bad = true; return false; }
        return true;
    }
    uint8_t u8() { return need(1) ? *p++ : 0; }
    uint32_t u32() { if (!need(4)) return 0; uint32_t v; std::memcpy(&v, p, 4); p += 4; return v; }
    uint64_t u64() { if (!need(8)) return 0; uint64_t v; std::memcpy(&v, p, 8); p += 8; return v; }
    double f64() { if (!need(8)) return 0; double v; std::memcpy(&v, p, 8); p += 8; return v; }
    int64_t varint() {
        uint64_t raw = 0;
        int shift = 0;
        while (true) {
            if (!need(1)) return 0;
            uint8_t b = *p++;
            raw |= uint64_t(b & 0x7F) << shift;
            if (!(b & 0x80)) break;
            shift += 7;
            if (shift > 63) { bad = true; return 0; }
        }
        return int64_t(raw >> 1) ^ -int64_t(raw & 1);   // zigzag
    }
    std::string text() {
        int64_t n = varint();
        if (n < 0 || !need(size_t(n))) { bad = true; return {}; }
        std::string s(reinterpret_cast<const char*>(p), size_t(n));
        p += n;
        return s;
    }
};

inline int lookup(const std::string& s, const char* const* table, int n, int fallback) {
    for (int i = 0; i < n; ++i)
        if (s == table[i]) return i;
    return fallback;
}

// One action element list: [name, maybe item-name, maybe count]. Mirrors
// export_trace.enc_unit / enc_order so the two encodings agree exactly.
struct Element { bool is_int; int64_t value; std::string name; };

// The official engine coerces with int(...), so a quantity recorded as the
// STRING "4" is a perfectly good 4. Real replays contain them - one episode has
// ["BUY_PRODUCT","FERTILIZER","4"] - and refusing the coercion here dropped the
// order, diverging from Python at step 530 of an otherwise clean episode.
inline bool numeric(const Element& e, int64_t& out) {
    if (e.is_int) { out = e.value; return true; }
    if (e.name.empty()) return false;
    size_t i = (e.name[0] == '-' || e.name[0] == '+') ? 1 : 0;
    if (i >= e.name.size()) return false;
    for (size_t k = i; k < e.name.size(); ++k)
        if (e.name[k] < '0' || e.name[k] > '9') return false;
    out = std::strtoll(e.name.c_str(), nullptr, 10);
    return true;
}

inline std::vector<Element> element_list(Cursor& c, const std::vector<std::string>& symbols) {
    int64_t n = c.varint();
    std::vector<Element> out;
    if (n < 0) { c.bad = true; return out; }
    out.reserve(size_t(n));
    for (int64_t i = 0; i < n && !c.bad; ++i) {
        uint8_t tag = c.u8();
        if (tag == ELEM_NULL) out.push_back({false, 0, {}});   // null: no name, not an int
        else if (tag == ELEM_INT) out.push_back({true, c.varint(), {}});
        else if (size_t(tag) < symbols.size()) out.push_back({false, 0, symbols[tag]});
        else { c.bad = true; }
    }
    return out;
}

inline kag::UnitAction to_unit(const std::vector<Element>& e) {
    if (e.empty() || e[0].is_int) return {kag::OP_PASS, 0, 1};
    kag::UnitAction u;
    u.op = uint8_t(lookup(e[0].name, OPS, 18, kag::OP_INVALID));
    u.arg = (e.size() >= 2 && !e[1].is_int)
            ? uint8_t(lookup(e[1].name, ITEMS, kag::N_ITEMS, 255)) : 0;
    int64_t n = 1;
    u.n = (e.size() >= 3 && numeric(e[2], n)) ? int32_t(n) : 1;
    return u;
}

inline kag::Order to_order(const std::vector<Element>& e) {
    if (e.empty() || e[0].is_int) return {kag::M_NONE, 0, 0};
    int op = lookup(e[0].name, MOPS, 7, 0);
    // HIRE and BUY_LAND carry no item or quantity even when a stray argument
    // was recorded, e.g. ["HIRE", 70] appears in real replays.
    if (op == kag::M_HIRE || op == kag::M_BUY_LAND) return {uint8_t(op), 0, 1};
    int64_t n = 0;
    if (e.size() < 3 || !numeric(e[2], n)) return {kag::M_NONE, 0, 0};
    uint8_t item = e[1].is_int ? 255 : uint8_t(lookup(e[1].name, ITEMS, kag::N_ITEMS, 255));
    return {uint8_t(op), item, int32_t(n)};
}

}  // namespace detail

// Returns false and reports on stderr if the file is malformed or was written
// for a different engine build.
inline bool load(const char* path, Episode& ep) {
    std::FILE* f = std::fopen(path, "rb");
    if (!f) { std::fprintf(stderr, "corpus: cannot open %s\n", path); return false; }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> raw(size_t(size < 0 ? 0 : size));
    size_t got = raw.empty() ? 0 : std::fread(raw.data(), 1, raw.size(), f);
    std::fclose(f);
    if (got != raw.size() || raw.size() < 8 || std::memcmp(raw.data(), "KAGZ", 4) != 0) {
        std::fprintf(stderr, "corpus: %s is not a .kagz file\n", path);
        return false;
    }

    detail::Cursor c{raw.data() + 4, raw.data() + raw.size()};
    uint8_t version = c.u8();
    if (version != 1) {
        std::fprintf(stderr, "corpus: %s has unsupported version %u\n", path, version);
        return false;
    }
    c.u8();                                   // flags
    ep.engine_version = c.text();
    if (!c.need(32)) return false;
    static const char* HEX = "0123456789abcdef";
    ep.engine_sha256.clear();
    for (int i = 0; i < 32; ++i) {
        ep.engine_sha256 += HEX[c.p[i] >> 4];
        ep.engine_sha256 += HEX[c.p[i] & 0xF];
    }
    c.p += 32;
    ep.episode_id = c.u64();
    ep.seed = c.u64();
    ep.n_steps = int(c.u32());
    ep.n_seats = int(c.u8());
    // Episode::actions is a fixed 2-seat array. Every Kaggriculture replay is
    // 2-seat, but fail loudly rather than writing past the array if that ever
    // stops being true.
    if (ep.n_seats != 2) {
        std::fprintf(stderr, "corpus: %s has %d seats, expected 2\n", path, ep.n_seats);
        return false;
    }

    ep.config.episode_steps = int(c.u32());
    ep.config.board_size = int(c.u32());
    ep.config.starting_money = int(c.u32());
    ep.config.max_orders = int(c.u32());
    ep.config.turns_per_day = int(c.u32());
    ep.config.shed_capacity = int(c.u32());
    ep.config.weed_chance = c.f64();
    ep.config.shop_unlock_interval = int(c.u32());
    ep.config.shop_sell_interval = int(c.u32());
    ep.config.center_sell_interval = int(c.u32());
    ep.config.hire_mult = int(c.u32());
    c.u32(); c.u32();                          // actTimeout, runTimeout: not engine inputs
    ep.config.seed = ep.seed;

    for (auto& t : ep.teams) t = c.text();
    for (auto& s : ep.statuses) s = c.text();
    for (auto& r : ep.rewards) r = c.f64();
    ep.submission_ids.clear();
    for (int64_t i = 0, n = c.varint(); i < n; ++i)
        ep.submission_ids.push_back(c.u64());
    c.text();                                                  // provenance blob

    std::vector<std::string> symbols;
    for (int64_t i = 0, n = c.varint(); i < n && !c.bad; ++i) symbols.push_back(c.text());

    ep.hashes.assign(size_t(ep.n_steps), 0);
    int64_t n_anchors = c.varint();
    for (int64_t i = 0; i < n_anchors && !c.bad; ++i) {
        uint32_t step = c.u32();
        uint64_t h = c.u64();
        if (step < ep.hashes.size()) ep.hashes[step] = h;
    }
    ep.chain_hash = c.u64();

    int64_t packed_len = c.varint();
    if (c.bad || packed_len < 0 || !c.need(size_t(packed_len))) {
        std::fprintf(stderr, "corpus: %s header is truncated\n", path);
        return false;
    }
    std::vector<uint8_t> body(size_t(packed_len) * 8 + (1u << 16));
    uLongf out_len = uLongf(body.size());
    while (true) {
        int rc = uncompress(body.data(), &out_len, c.p, uLong(packed_len));
        if (rc == Z_OK) break;
        if (rc != Z_BUF_ERROR) {
            std::fprintf(stderr, "corpus: %s body inflate failed (%d)\n", path, rc);
            return false;
        }
        body.resize(body.size() * 2);
        out_len = uLongf(body.size());
    }

    detail::Cursor b{body.data(), body.data() + out_len};
    ep.actions.assign(size_t(ep.n_steps), {});
    for (int t = 0; t < ep.n_steps && !b.bad; ++t) {
        for (int seat = 0; seat < ep.n_seats && !b.bad; ++seat) {
            uint8_t present = b.u8();
            kag::Action& a = ep.actions[size_t(t)][size_t(seat)];
            a.clear();
            int n_units = 0;
            if (present & 1) {
                auto farmer = detail::element_list(b, symbols);
                a.units[n_units++] = detail::to_unit(farmer);
            } else {
                a.units[n_units++] = {kag::OP_PASS, 0, 1};
            }
            for (int64_t i = 0, n = b.varint(); i < n && !b.bad; ++i) {
                auto hand = detail::element_list(b, symbols);
                if (n_units < kag::MAX_UNITS) a.units[n_units++] = detail::to_unit(hand);
            }
            a.n_units = n_units;
            int n_orders = 0;
            for (int64_t i = 0, n = b.varint(); i < n && !b.bad; ++i) {
                auto order = detail::element_list(b, symbols);
                if (n_orders < MAX_ORDERS) a.orders[n_orders++] = detail::to_order(order);
            }
            a.n_orders = n_orders;
            a.finalize();
        }
    }
    if (b.bad || c.bad) {
        std::fprintf(stderr, "corpus: %s action stream is malformed\n", path);
        return false;
    }
    return true;
}

inline bool load(const std::string& path, Episode& ep) { return load(path.c_str(), ep); }

// Re-simulate and check every canonical state hash. This is the same digest
// fast_game_engine/validate checks, so a pass here means the container
// reproduces the recorded episode exactly.
inline bool verify(const Episode& ep, int* first_mismatch = nullptr) {
    kag::Sim sim(ep.config);
    for (int t = 0; t < ep.n_steps; ++t) {
        if (sim.parity_hash() != ep.hashes[size_t(t)]) {
            if (first_mismatch) *first_mismatch = t;
            return false;
        }
        if (t + 1 < ep.n_steps) sim.step(ep.actions[size_t(t) + 1][0], ep.actions[size_t(t) + 1][1]);
    }
    if (first_mismatch) *first_mismatch = -1;
    return sim.reward(0) == ep.rewards[0] && sim.reward(1) == ep.rewards[1];
}

// Advance a fresh Sim to the start of `day`, so callers can hand that state and
// the day's 24 hourly actions to the day-contract extractor.
inline kag::Sim at_day(const Episode& ep, int day) {
    kag::Sim sim(ep.config);
    int target = day * ep.config.turns_per_day;
    for (int t = 0; t < target && t + 1 < ep.n_steps; ++t)
        sim.step(ep.actions[size_t(t) + 1][0], ep.actions[size_t(t) + 1][1]);
    return sim;
}

}  // namespace corpus
