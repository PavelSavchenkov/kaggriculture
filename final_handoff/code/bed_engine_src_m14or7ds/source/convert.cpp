#include "convert.hpp"

namespace dc10 {
namespace {

bool same_entity(const Tile& a, const Tile& b) {
    return a.kind == b.kind && a.what == b.what && a.has_animal == b.has_animal &&
           a.planted_day == b.planted_day;
}

struct Facts {
    bool watered = false, fertilized = false, harvested = false, dug = false;
    bool fed = false, cared = false;
};

struct DayFacts {
    std::array<Facts, BOARD * BOARD> dawn{}, created{};
    std::array<bool, BOARD * BOARD> tile_dug{};  // any successful DIG on the tile
};

bool farm_op(uint8_t op) {
    return op == OP_PLANT || op == OP_WATER || op == OP_HARVEST || op == OP_FERTILIZE || op == OP_DIG ||
           op == OP_PLACE || op == OP_FEED || op == OP_CARE;
}

// Replays one step unit by unit and records each successful effect on our farm.
void record_step(const Sim& state, const std::array<Action, 2>& joint, int seat, int day,
                 const Farm& dawn_farm, DayFacts& facts) {
    const Action& own = joint[seat];
    const Farm& start = state.st.farms[seat];
    const int units = std::min(own.n_units, start.n_units);
    Action prefix = own;  // keeps the full action's atomic PLANT demand metadata
    prefix.n_orders = 0;
    for (int u = 0; u < prefix.n_units; ++u) prefix.units[u] = UnitAction{};
    Action none;
    none.finalize();
    auto run = [&](const Action& mine) {
        return seat == 0 ? step_without_night(state, mine, none) : step_without_night(state, none, mine);
    };
    Sim before = run(prefix);
    for (int u = 0; u < units; ++u) {
        prefix.units[u] = own.units[u];
        if (!farm_op(own.units[u].op)) continue;
        Sim after = run(prefix);
        const Farm& fb = before.st.farms[seat];
        const Farm& fa = after.st.farms[seat];
        const int x = start.pos_x[u], y = start.pos_y[u];
        const int cell = cell_of(x, y);
        const Tile& tb = fb.tiles[y][x];
        const Tile& ta = fa.tiles[y][x];
        const Tile& td = dawn_farm.tiles[y][x];
        Facts* f = nullptr;
        if ((tb.kind == T_PLANT || tb.has_animal) && same_entity(td, tb)) f = &facts.dawn[cell];
        else if ((tb.kind == T_PLANT || tb.has_animal) && tb.planted_day == day) f = &facts.created[cell];
        const auto op = own.units[u].op;
        if (op == OP_PLANT && tb.kind == T_EMPTY && ta.kind == T_PLANT) facts.created[cell] = {};
        if (op == OP_PLACE && !tb.has_animal && ta.has_animal) facts.created[cell] = {};
        if (op == OP_DIG && tb.kind != T_EMPTY && !tb.has_animal && ta.kind == T_EMPTY) {
            facts.tile_dug[cell] = true;
            if (f && tb.kind == T_PLANT) f->dug = true;
        }
        if (f) {
            if (op == OP_WATER && ta.watered_today && !tb.watered_today) f->watered = true;
            if (op == OP_FERTILIZE && fa.inv[u][FERTILIZER] < fb.inv[u][FERTILIZER] &&
                ta.fertilized_until_day > tb.fertilized_until_day)
                f->fertilized = true;
            if (op == OP_HARVEST) {
                const int product = tb.kind == T_PLANT ? int(tb.what) : int(ANIMALS[tb.what - GOOSE].product);
                if (fa.inv[u][product] > fb.inv[u][product]) f->harvested = true;
            }
            if (op == OP_FEED && ta.fed_today && !tb.fed_today) f->fed = true;
            if (op == OP_CARE && ta.cared_today && !tb.cared_today) f->cared = true;
        }
        before = std::move(after);
    }
}

// Drops actions that a fixed-value rule excludes: first fertilizer, then water.
int normalize_option(const CropGroup& g, int day, bool water, bool fertilize, bool harvest, int& dropped) {
    int o = 4 * water + 2 * fertilize + harvest;
    if (option_fixed(g, day, o) && fertilize) o &= ~2, ++dropped;
    if (option_fixed(g, day, o) && (o & 4)) o &= ~4, ++dropped;
    if (option_fixed(g, day, o) && (o & 2)) o &= ~2, ++dropped;
    return o;
}
}

Conversion convert_day(const std::vector<Sim>& states, const std::vector<std::array<Action, 2>>& turns,
                       int seat, int day) {
    const int steps = day == LAST_DAY ? int(turns.size()) - day * HOURS : HOURS;
    return convert_steps(&states[day * HOURS], &turns[day * HOURS], steps, seat);
}

Conversion convert_steps(const Sim* states, const std::array<Action, 2>* turns, int steps, int seat) {
    Conversion out;
    const Sim& dawn_sim = states[0];
    const int day = dawn_sim.st.day;
    const bool terminal = day == LAST_DAY;
    const int end_step = steps;
    const Farm& dawn_farm = dawn_sim.st.farms[seat];
    const Farm& end_farm = states[end_step].st.farms[seat];

    const int land = end_farm.n_quadrants - dawn_farm.n_quadrants;
    if (land > 1) { out.status = ConvertStatus::IgnoredLand; return out; }
    for (int a = 0; a < N_ANIMALS; ++a)
        if (end_farm.discarded[GOOSE + a] > dawn_farm.discarded[GOOSE + a]) {
            out.status = ConvertStatus::IgnoredDiscard;
            return out;
        }

    DayFacts facts;
    for (int step = 0; step < end_step; ++step)
        record_step(states[step], turns[step], seat, day, dawn_farm, facts);

    const auto dawn = agent::runtime::make_observation(dawn_sim, seat);
    out.schema = describe(dawn);
    Schema& s = out.schema;
    DayIntent& in = out.intent;
    in.buy_land = land == 1;
    auto fail = [&](int cell, const std::string& why) {
        out.status = ConvertStatus::Failed;
        out.failure = why + " at " + std::to_string(cell % BOARD) + "," + std::to_string(cell / BOARD);
        return out;
    };

    // Existing crop groups.
    for (int i = 0; i < s.n_crops; ++i) {
        const CropGroup& g = s.crops[i];
        if (g.fresh) continue;
        for (int m = 0; m < g.size; ++m) {
            const int cell = g.cells[m];
            const Facts& f = facts.dawn[cell];
            const bool retained = !terminal && same_entity(dawn_farm.tiles[cell_y(cell)][cell_x(cell)],
                                                           end_farm.tiles[cell_y(cell)][cell_x(cell)]);
            if (!CROPS[g.crop].ongoing) {
                int o;
                if (f.dug && !f.harvested) {
                    o = CLEAR;
                    if (option_fixed(g, day, o)) o = 0, ++out.dropped_actions;
                } else {
                    o = normalize_option(g, day, f.watered, f.fertilized, f.harvested, out.dropped_actions);
                }
                if (option_fixed(g, day, o)) return fail(cell, "one-shot option " + std::to_string(o) + " is fixed");
                ++in.options[i][o];
                continue;
            }
            in.harvest[i] += f.harvested;
            if (terminal) {
                out.dropped_actions += f.fertilized + facts.tile_dug[cell];
                continue;
            }
            const bool cleared = !retained && facts.tile_dug[cell];
            const Tile& end = end_farm.tiles[cell_y(cell)][cell_x(cell)];
            if (!retained && !cleared && end.kind != T_WEED)
                return fail(cell, "ongoing crop left without clear or weed");
            in.retain[i] += retained;
            in.clear[i] += cleared;
            if (f.fertilized && retained) {
                if (ongoing_fertilize_fixed(g, day)) ++out.dropped_actions;
                else ++in.fertilize[i];
            }
        }
    }
    // Existing animal groups.
    for (int i = 0; i < s.n_animals; ++i) {
        const AnimalGroup& g = s.animals[i];
        if (g.fresh) continue;
        for (int m = 0; m < g.size; ++m) {
            const int cell = g.cells[m];
            const Facts& f = facts.dawn[cell];
            in.collect[i] += f.harvested;
            if (terminal) {
                out.dropped_actions += f.fed + f.cared;
                continue;
            }
            const bool retained = same_entity(dawn_farm.tiles[cell_y(cell)][cell_x(cell)],
                                              end_farm.tiles[cell_y(cell)][cell_x(cell)]);
            if (retained != (g.unfed == 0 || f.fed)) return fail(cell, "animal retention differs from feed rule");
            in.feed[i] += f.fed;
            if (f.fed && f.cared) {
                if (care_fixed(g, day)) ++out.dropped_actions;
                else ++in.care[i];
            }
        }
    }
    // New entities: alive after tonight. Day-29 plantings and placements are dropped.
    for (int cell = 0; cell < BOARD * BOARD; ++cell) {
        const Tile& end = end_farm.tiles[cell_y(cell)][cell_x(cell)];
        const Facts& f = facts.created[cell];
        if (terminal || end.planted_day != day) continue;
        if (end.kind == T_PLANT) {
            const int gi = s.new_crop_group[end.what];
            CropGroup& g = s.crops[gi];
            ++in.new_crop[end.what];
            if (CROPS[end.what].ongoing) continue;
            const int o = normalize_option(g, day, f.watered, f.fertilized, false, out.dropped_actions);
            if (option_fixed(g, day, o)) return fail(cell, "new one-shot option " + std::to_string(o) + " is fixed");
            ++in.options[gi][o];
        } else if (end.has_animal) {
            const int species = end.what - GOOSE;
            const int gi = s.new_animal_group[species];
            ++in.new_animal[species];
            in.feed[gi] += f.fed;
            if (f.fed && f.cared) {
                if (care_fixed(s.animals[gi], day)) ++out.dropped_actions;
                else ++in.care[gi];
            }
        }
    }
    for (int a = 0; a < N_ANIMALS; ++a)
        in.reserve[a] = int16_t(terminal ? s.unplaced_dawn[a] : end_farm.shed[GOOSE + a]);
    size_new_groups(s, in);
    const std::string why = validate(s, in);
    if (!why.empty()) {
        out.status = ConvertStatus::Failed;
        out.failure = "label invalid: " + why;
    }
    return out;
}
}

namespace dc10 {
int intent_distance(const Schema& s, const DayIntent& a, const DayIntent& b) {
    int d = a.buy_land != b.buy_land;
    for (int i = 0; i < N_CROPS; ++i) d += std::abs(a.new_crop[i] - b.new_crop[i]);
    for (int i = 0; i < N_ANIMALS; ++i)
        d += std::abs(a.new_animal[i] - b.new_animal[i]) + std::abs(a.reserve[i] - b.reserve[i]);
    for (int i = 0; i < s.n_crops; ++i) {
        for (int o = 0; o < OPTIONS; ++o) d += std::abs(a.options[i][o] - b.options[i][o]);
        d += std::abs(a.retain[i] - b.retain[i]) + std::abs(a.clear[i] - b.clear[i]) +
             std::abs(a.fertilize[i] - b.fertilize[i]) + std::abs(a.harvest[i] - b.harvest[i]);
    }
    for (int i = 0; i < s.n_animals; ++i)
        d += std::abs(a.feed[i] - b.feed[i]) + std::abs(a.care[i] - b.care[i]) + std::abs(a.collect[i] - b.collect[i]);
    return d;
}
}
