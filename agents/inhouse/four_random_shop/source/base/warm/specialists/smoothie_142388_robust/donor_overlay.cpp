#include "agents/inhouse/four_random_shop/source/base/warm/specialists/smoothie_142388_robust/donor_overlay.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>

#include "fast_game_engine/sim.hpp"

namespace kag::agents::four_random_shop::base::warm::smoothie_142388_robust::detail::donor {
namespace tape {
#ifndef FIXED_WEED_DONOR_TAPE_INCLUDE
#define FIXED_WEED_DONOR_TAPE_INCLUDE "agents/inhouse/four_random_shop/source/base/warm/specialists/smoothie_142388_robust/tape.inc"
#endif
#include FIXED_WEED_DONOR_TAPE_INCLUDE
}
namespace {

struct Commitment {
    int16_t step = 0;
    int8_t x = 0;
    int8_t y = 0;
    uint8_t kind = 0;
};

struct NominalTable {
    std::array<Commitment, 256> commitments{};
    int commitment_count = 0;
    std::array<std::array<int8_t, kag::MAX_UNITS>, tape::TAPE_STEPS + 1> x{};
    std::array<std::array<int8_t, kag::MAX_UNITS>, tape::TAPE_STEPS + 1> y{};
    std::array<uint8_t, tape::TAPE_STEPS + 1> units{};
};

kag::UnitAction nominal_unit(int step, int unit) {
    if (step < 0 || step >= tape::TAPE_STEPS) return {};
    int cursor = tape::TAPE_OFFSETS[step];
    const int units = tape::TAPE_DATA[cursor++];
    ++cursor;
    if (unit < 0 || unit >= units) return {};
    cursor += unit * 3;
    return {static_cast<uint8_t>(tape::TAPE_DATA[cursor]),
        static_cast<uint8_t>(tape::TAPE_DATA[cursor + 1]),
        tape::TAPE_DATA[cursor + 2]};
}

int nominal_hires(int step) {
    int cursor = tape::TAPE_OFFSETS[step];
    const int units = tape::TAPE_DATA[cursor++];
    const int orders = tape::TAPE_DATA[cursor++];
    cursor += units * 3;
    int hires = 0;
    for (int order = 0; order < orders; ++order) {
        hires += tape::TAPE_DATA[cursor] == kag::M_HIRE;
        cursor += 3;
    }
    return hires;
}

void nominal_action(int step, int live_units, kag::Action& action) {
    action.clear();
    action.n_units = live_units;
    int cursor = tape::TAPE_OFFSETS[step];
    const int units = tape::TAPE_DATA[cursor++];
    const int orders = tape::TAPE_DATA[cursor++];
    for (int unit = 0; unit < units; ++unit) {
        const kag::UnitAction selected = {
            static_cast<uint8_t>(tape::TAPE_DATA[cursor]),
            static_cast<uint8_t>(tape::TAPE_DATA[cursor + 1]),
            tape::TAPE_DATA[cursor + 2]};
        cursor += 3;
        if (unit < live_units) action.units[unit] = selected;
    }
    action.n_orders = std::min(orders, 10);
    for (int order = 0; order < orders; ++order) {
        const kag::Order selected = {
            static_cast<uint8_t>(tape::TAPE_DATA[cursor]),
            static_cast<uint8_t>(tape::TAPE_DATA[cursor + 1]),
            tape::TAPE_DATA[cursor + 2]};
        cursor += 3;
        if (order < action.n_orders) action.orders[order] = selected;
    }
    action.finalize();
}

bool movement_or_pass(uint8_t op) {
    return op == kag::OP_PASS || (op >= kag::OP_NORTH && op <= kag::OP_WEST);
}

const NominalTable& nominal_table() {
    static const NominalTable table = [] {
        NominalTable result;
        kag::Config config;
        config.seed = 1;
        config.shop_unlock_interval = 1000;
        config.weed_chance = 0;
        kag::Sim sim(config);
        kag::Action pass;
        pass.clear();
        pass.finalize();
        for (int step = 0; step < tape::TAPE_STEPS; ++step) {
            const kag::Farm& farm = sim.st.farms[0];
            result.units[step] = static_cast<uint8_t>(farm.n_units);
            for (int unit = 0; unit < farm.n_units; ++unit) {
                result.x[step][unit] = farm.pos_x[unit];
                result.y[step][unit] = farm.pos_y[unit];
            }
            kag::Action action;
            nominal_action(step, farm.n_units, action);
            for (int unit = 0; unit < farm.n_units; ++unit) {
                const kag::UnitAction selected = action.units[unit];
                if (selected.op != kag::OP_PLANT && selected.op != kag::OP_BUILD_COOP &&
                    selected.op != kag::OP_BUILD_PASTURE)
                    continue;
                if (result.commitment_count >= static_cast<int>(result.commitments.size()))
                    std::abort();
                const uint8_t kind = selected.op == kag::OP_PLANT ? selected.arg :
                    kag::N_CROPS + (selected.op == kag::OP_BUILD_PASTURE);
                result.commitments[result.commitment_count++] = {
                    static_cast<int16_t>(step), farm.pos_x[unit], farm.pos_y[unit], kind};
            }
            sim.step(action, pass);
        }
        result.units[tape::TAPE_STEPS] = static_cast<uint8_t>(sim.st.farms[0].n_units);
        return result;
    }();
    return table;
}

int distance(int x0, int y0, int x1, int y1) {
    return std::abs(x0 - x1) + std::abs(y0 - y1);
}

void append_moves(std::array<kag::UnitAction, 24>& actions,
                  int& size, int& x, int& y, int tx, int ty) {
    while (x != tx) {
        actions[size++] = {static_cast<uint8_t>(x < tx ? kag::OP_EAST : kag::OP_WEST), 0, 1};
        x += x < tx ? 1 : -1;
    }
    while (y != ty) {
        actions[size++] = {static_cast<uint8_t>(y < ty ? kag::OP_SOUTH : kag::OP_NORTH), 0, 1};
        y += y < ty ? 1 : -1;
    }
}

}  // namespace

void DonorOverlay::reset(uint8_t player) {
    player_ = player;
    last_day_ = -1;
    stats_ = {};
    target_count_ = 0;
    for (Detour& detour : detours_) detour.clear();
}

void DonorOverlay::begin_day(const kag::agent::AgentObservation& observation) {
    last_day_ = observation.day;
    target_count_ = 0;
    for (Detour& detour : detours_) detour.clear();
    if (!parameters_.enabled) return;
    const NominalTable& table = nominal_table();
    for (int index = 0; index < table.commitment_count; ++index) {
        const Commitment& commitment = table.commitments[index];
        const int commitment_day = commitment.step / 24;
        if (commitment_day < observation.day ||
            commitment_day - observation.day > parameters_.maximum_lead_days ||
            !(parameters_.day_mask & (uint32_t{1} << commitment_day)) ||
            !(parameters_.kind_mask & (uint32_t{1} << commitment.kind)) ||
            observation.self().tiles[commitment.y][commitment.x].kind != kag::T_WEED)
            continue;
        int existing = -1;
        for (int target = 0; target < target_count_; ++target)
            if (targets_[target].x == commitment.x && targets_[target].y == commitment.y)
                existing = target;
        if (existing >= 0) {
            targets_[existing].deadline = std::min(targets_[existing].deadline,
                commitment.step);
            continue;
        }
        if (target_count_ >= MAX_TARGETS) std::abort();
        targets_[target_count_++] = {
            commitment.step, commitment.x, commitment.y, commitment.kind, true, false};
    }
    stats_.targets += target_count_;
}

bool DonorOverlay::plan(const kag::agent::AgentObservation& observation,
                        const kag::Action& base, int unit) {
    const NominalTable& table = nominal_table();
    const int step = observation.step;
    if (unit >= table.units[step] ||
        observation.self().pos_x[unit] != table.x[step][unit] ||
        observation.self().pos_y[unit] != table.y[step][unit] ||
        base.units[unit].op != nominal_unit(step, unit).op ||
        base.units[unit].arg != nominal_unit(step, unit).arg ||
        base.units[unit].n != nominal_unit(step, unit).n)
        return false;

    int best_target = -1;
    int best_rejoin = -1;
    int best_slots = std::numeric_limits<int>::max();
    const int day_end = std::min(tape::TAPE_STEPS, (observation.day + 1) * 24);
    for (int target_index = 0; target_index < target_count_; ++target_index) {
        Target& target = targets_[target_index];
        if (!target.active || target.reserved || target.deadline <= step) continue;
        if (observation.self().tiles[target.y][target.x].kind != kag::T_WEED) {
            target.active = false;
            continue;
        }
        const int outward = distance(table.x[step][unit], table.y[step][unit],
            target.x, target.y);
        if (step + outward >= target.deadline) continue;
        bool safe = true;
        for (int rejoin = step + 1; rejoin <= day_end; ++rejoin) {
            safe &= movement_or_pass(nominal_unit(rejoin - 1, unit).op);
            if (!safe || unit >= table.units[rejoin]) break;
            const int required = outward + 1 + distance(target.x, target.y,
                table.x[rejoin][unit], table.y[rejoin][unit]);
            const int slots = rejoin - step;
            if (required > slots || slots - required < parameters_.minimum_slack) continue;
            ++stats_.available_windows;
            if (slots < best_slots ||
                (slots == best_slots && target.deadline < targets_[best_target].deadline)) {
                best_target = target_index;
                best_rejoin = rejoin;
                best_slots = slots;
            }
            break;
        }
    }
    if (best_target < 0) return false;

    Detour& detour = detours_[unit];
    detour.clear();
    int x = table.x[step][unit];
    int y = table.y[step][unit];
    const Target& target = targets_[best_target];
    int size = 0;
    append_moves(detour.actions, size, x, y, target.x, target.y);
    detour.actions[size++] = {kag::OP_DIG, 0, 1};
    append_moves(detour.actions, size, x, y,
        table.x[best_rejoin][unit], table.y[best_rejoin][unit]);
    while (size < best_rejoin - step) detour.actions[size++] = {};
    if (size > MAX_ROUTE) std::abort();
    detour.size = static_cast<uint8_t>(size);
    detour.target = static_cast<int8_t>(best_target);
    targets_[best_target].reserved = true;
    ++stats_.plans;
    stats_.route_steps += size;
    if (stats_.event_count < static_cast<int>(stats_.events.size()))
        stats_.events[stats_.event_count++] = {
            static_cast<int16_t>(step), target.deadline,
            static_cast<int16_t>(best_rejoin), static_cast<int8_t>(unit),
            target.x, target.y};
    return true;
}

void DonorOverlay::modify(const kag::agent::AgentObservation& observation,
                          kag::Action& action) {
    if (observation.player != player_) std::abort();
    if (observation.day != last_day_) begin_day(observation);
    if (!parameters_.enabled) return;

    for (int unit = 0; unit < action.n_units; ++unit) {
        Detour& detour = detours_[unit];
        if (!detour.active()) plan(observation, action, unit);
        if (!detour.active()) continue;
        const kag::UnitAction selected = detour.actions[detour.cursor++];
        action.units[unit] = selected;
        if (selected.op == kag::OP_DIG) {
            ++stats_.digs;
            if (detour.target >= 0) targets_[detour.target].active = false;
        }
        if (!detour.active()) detour.clear();
    }
    if (parameters_.suppress_redundant_cleanup_hire && target_count_ > 0) {
        bool all_reserved = true;
        for (int target = 0; target < target_count_; ++target)
            all_reserved &= !targets_[target].active || targets_[target].reserved;
        int action_hires = 0;
        for (int order = 0; order < action.n_orders; ++order)
            action_hires += action.orders[order].op == kag::M_HIRE;
        if (all_reserved && action_hires > nominal_hires(observation.step)) {
            int remove = action.n_orders - 1;
            while (remove >= 0 && action.orders[remove].op != kag::M_HIRE) --remove;
            if (remove < 0) std::abort();
            for (int order = remove + 1; order < action.n_orders; ++order)
                action.orders[order - 1] = action.orders[order];
            --action.n_orders;
            ++stats_.suppressed_hires;
        }
    }
    action.finalize();
}

}  // namespace kag::agents::four_random_shop::base::warm::smoothie_142388_robust::detail::donor
