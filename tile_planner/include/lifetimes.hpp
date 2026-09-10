#pragma once
#include "course.hpp"
#include "extract_contract.hpp"

namespace placement {
constexpr uint32_t all_days = (uint32_t{1} << 30) - 1;
struct LifeSpec {
    int item, begin_day, stop_day = 30;
    uint32_t water = all_days, feed = all_days, care = all_days;
    uint32_t collect = all_days, harvest = all_days, fertilize = 0;
    std::array<bool, 100> allowed{};
    bool clear_after_service = false;
    // Optional four-bit codes in execution order: water/feed/care/collect/
    // harvest/fertilize = 1..6. Zero uses the normal biological order.
    std::array<uint32_t, 30> service_order{};
    bool repeated_service = false;
    friend bool operator==(const LifeSpec&, const LifeSpec&) = default;
};
struct LifeSpecHash {
    size_t operator()(const LifeSpec& s) const {
        uint64_t value = 1469598103934665603ULL;
        auto add = [&](uint32_t field) { value ^= field; value *= 1099511628211ULL; };
        add(s.item); add(s.begin_day); add(s.stop_day);
        add(s.water); add(s.feed); add(s.care); add(s.collect); add(s.harvest); add(s.fertilize);
        for (bool cell : s.allowed) add(cell);
        add(s.clear_after_service);
        for (auto order : s.service_order) add(order);
        add(s.repeated_service);
        return value;
    }
};
struct LifeDay {
    day_solver::ManagedTileState before, after_work, after_night;
    std::vector<day_solver::TileWorkAction> work;
};
struct LifeProgram {
    LifeSpec spec;
    std::array<LifeDay, 30> days;
    int release_day = 30;
};
using LifeAssignment = std::vector<int>;

inline LifeSpec productive_life(int item, int begin, int stop = 30) {
    LifeSpec result{item, begin, stop};
    for (int i = 0; i < 100; ++i) result.allowed[i] = quadrant(i) == 0;
    if (kag::is_crop(item) && !kag::CROPS[item].ongoing)
        result.harvest = uint32_t{1} << std::min(stop - 1, begin + kag::CROPS[item].max_yield_day);
    if (kag::is_crop(item) && kag::CROPS[item].ongoing)
        result.stop_day = std::min(stop, begin + kag::CROPS[item].first_yield_day + (kag::CROPS[item].max_yield - 1) * kag::CROPS[item].interval + 1);
    return result;
}

inline bool living(const day_solver::ManagedTileState& state) {
    return state.kind == day_solver::ManagedTileKind::CROP || state.animal >= 0;
}

// A single-tile biological oracle. Injected seed/cargo supplies isolate biology;
// this is never a route, cash or complete-game feasibility certificate.
inline LifeProgram compile_life(const LifeSpec& spec) {
    if (spec.begin_day < 0 || spec.begin_day >= 30 || spec.begin_day > spec.stop_day || (spec.begin_day == spec.stop_day && !spec.clear_after_service) || spec.stop_day > 30 ||
        !(kag::is_crop(spec.item) || kag::is_animal(spec.item))) throw std::runtime_error("invalid life");
    LifeProgram result; result.spec = spec;
    kag::Config config; config.weed_chance = 0; config.shed_capacity = 30000; config.episode_steps = 721;
    kag::Sim sim(config);
    kag::Action pass; pass.n_units = 1; pass.finalize();
    bool born = false, released = false;
    for (int d = 0; d < 30; ++d) {
        auto& row = result.days[d]; auto& farm = sim.st.farms[0];
        row.before = labor::offline::managed(farm.tiles[4][4], d);
        auto on = [d](uint32_t mask) { return (mask >> d) & 1; };
        auto act = [&](int op, int arg = day_solver::NO_SUBJECT) {
            if (sim.st.hour >= 22) throw std::runtime_error("too many single-tile biological operations");
            if (op == kag::OP_PLANT) ++farm.seeds[arg];
            if (op == kag::OP_PLACE) farm.inv_add(0, arg, 1);
            if (op == kag::OP_FEED) farm.inv_add(0, kag::WHEAT, 1);
            if (op == kag::OP_FERTILIZE) farm.inv_add(0, kag::FERTILIZER, 1);
            std::array<int, kag::N_ITEMS> produced{}; std::copy_n(farm.produced, kag::N_ITEMS, produced.begin());
            kag::Action action; action.n_units = 1;
            action.units[0] = {uint8_t(op), uint8_t(arg < 0 ? 0 : arg), 1}; action.finalize();
            const auto diagnosis = sim.diagnose_joint_actions(action, pass).players[0];
            if (diagnosis.requested_unit_actions != diagnosis.successful_unit_actions) throw std::runtime_error("invalid biological operation");
            sim.step(action, pass);
            day_solver::TileWorkAction work; work.op = op; work.arg = arg;
            for (int item = 0; item < kag::N_ITEMS; ++item) if (farm.produced[item] > produced[item]) {
                if (work.output_item >= 0) throw std::runtime_error("multiple biological outputs");
                work.output_item = item; work.output_quantity = farm.produced[item] - produced[item];
                if (op == kag::OP_HARVEST) work.arg = item;
            }
            row.work.push_back(work);
        };
        if (d == spec.begin_day) {
            born = true;
            if (kag::is_animal(spec.item)) {
                act(spec.item == kag::GOOSE ? kag::OP_BUILD_COOP : kag::OP_BUILD_PASTURE);
                act(kag::OP_PLACE, spec.item);
            } else act(kag::OP_PLANT, spec.item);
        }
        auto& tile = farm.tiles[4][4];
        if (born && !released && d == spec.stop_day && !spec.clear_after_service) {
            if (tile.has_animal) throw std::runtime_error("cannot remove an established animal at stop_day");
            if (tile.kind != kag::T_EMPTY) act(kag::OP_DIG);
        }
        if (born && !released && (d < spec.stop_day || (d == spec.stop_day && spec.clear_after_service))) {
            auto service = [&](int code) {
                if (code == 1 && tile.kind == kag::T_PLANT && on(spec.water)) { act(kag::OP_WATER); return true; }
                if (code == 2 && tile.has_animal && on(spec.feed)) { act(kag::OP_FEED); return true; }
                if (code == 3 && tile.has_animal && on(spec.care)) { act(kag::OP_CARE); return true; }
                if (code == 4 && tile.has_animal && on(spec.collect) && tile.fertilizer_available) { act(kag::OP_COLLECT_FERTILIZER); return true; }
                if (code == 5 && on(spec.harvest) && tile.yield_units > 0 &&
                    (tile.has_animal || (tile.kind == kag::T_PLANT && d - tile.planted_day >= kag::CROPS[spec.item].first_yield_day))) { act(kag::OP_HARVEST); return true; }
                if (code == 6 && tile.kind == kag::T_PLANT && on(spec.fertilize)) { act(kag::OP_FERTILIZE); return true; }
                return false;
            };
            if (uint32_t ordered = spec.service_order[d]) {
                unsigned required = 0;
                if (tile.kind == kag::T_PLANT) {
                    if (on(spec.water)) required |= 1u << 1;
                    if (on(spec.fertilize)) required |= 1u << 6;
                    if (on(spec.harvest) && tile.yield_units > 0 && d - tile.planted_day >= kag::CROPS[spec.item].first_yield_day) required |= 1u << 5;
                } else if (tile.has_animal) {
                    if (on(spec.feed)) required |= 1u << 2;
                    if (on(spec.care)) required |= 1u << 3;
                    if (on(spec.collect) && tile.fertilizer_available) required |= 1u << 4;
                    if (on(spec.harvest) && tile.yield_units > 0) required |= 1u << 5;
                }
                unsigned seen = 0;
                while (ordered) {
                    const int code = ordered & 15; ordered >>= 4;
                    if (code < 1 || code > 6 || (!spec.repeated_service && (seen & (1u << code))) || !service(code)) throw std::runtime_error("invalid explicit biological service order");
                    seen |= 1u << code;
                }
                if (seen != required) throw std::runtime_error("explicit service order omits a required operation");
            } else for (int code : {6, 1, 2, 3, 4, 5}) service(code);
        }
        if (born && !released && d == spec.stop_day && spec.clear_after_service) {
            if (tile.has_animal) throw std::runtime_error("cannot clear an established animal after service");
            if (tile.kind != kag::T_EMPTY) act(kag::OP_DIG);
        }
        row.after_work = labor::offline::managed(tile, d);
        if (born && !released && !living(row.after_work)) { result.release_day = d; released = true; }
        while (sim.st.day == d) sim.step(pass, pass);
        row.after_night = labor::offline::managed(farm.tiles[4][4], d + 1);
        if (born && !released && !living(row.after_night)) { result.release_day = d + 1; released = true; }
    }
    return result;
}

inline bool legal_lives(const std::vector<LifeProgram>& lives, const LifeAssignment& assignment) {
    if (lives.size() != assignment.size()) return false;
    std::array<std::vector<int>, 100> cells;
    for (size_t i = 0; i < lives.size(); ++i) {
        const int cell = assignment[i];
        if (cell < 0 || cell >= 100 || !lives[i].spec.allowed[cell]) return false;
        cells[cell].push_back(i);
    }
    for (auto& chain : cells) {
        std::stable_sort(chain.begin(), chain.end(), [&](int a, int b) { return lives[a].spec.begin_day < lives[b].spec.begin_day; });
        for (size_t i = 1; i < chain.size(); ++i) {
            const auto& prior = lives[chain[i - 1]]; const auto& next = lives[chain[i]];
            if (prior.spec.begin_day == next.spec.begin_day || prior.release_day > next.spec.begin_day) return false;
        }
    }
    return true;
}

inline std::optional<LifeAssignment> try_assign_lives(const std::vector<LifeProgram>& lives, bool animals_first, bool nearest) {
    LifeAssignment result(lives.size(), -1);
    std::vector<int> order(lives.size()); std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
        if (animals_first && kag::is_animal(lives[a].spec.item) != kag::is_animal(lives[b].spec.item)) return kag::is_animal(lives[a].spec.item);
        return lives[a].spec.begin_day < lives[b].spec.begin_day;
    });
    for (int id : order) {
        std::vector<int> cells;
        for (int cell = 0; cell < 100; ++cell) if (lives[id].spec.allowed[cell]) cells.push_back(cell);
        if (nearest) std::stable_sort(cells.begin(), cells.end(), [](int a, int b) { return std::pair{labor::shed_distance(a), a} < std::pair{labor::shed_distance(b), b}; });
        for (int cell : cells) {
            bool compatible = true;
            for (size_t j = 0; j < lives.size(); ++j) if (result[j] == cell) {
                const auto& a = lives[id]; const auto& b = lives[j];
                compatible &= (a.spec.begin_day < b.spec.begin_day && a.release_day <= b.spec.begin_day) ||
                              (b.spec.begin_day < a.spec.begin_day && b.release_day <= a.spec.begin_day);
            }
            if (compatible) { result[id] = cell; break; }
        }
        if (result[id] < 0) return std::nullopt;
    }
    if (!legal_lives(lives, result)) throw std::runtime_error("invalid constructed life assignment");
    return result;
}

inline LifeAssignment assign_lives(const std::vector<LifeProgram>& lives, bool animals_first, bool nearest) {
    const auto result = try_assign_lives(lives, animals_first, nearest);
    if (!result) throw std::runtime_error("greedy life assignment exhausted allowed cells");
    return *result;
}

struct LifeDayContract { Day day; std::array<int64_t, kag::N_ITEMS> output{}; int clearing_actions = 0, reused_structures = 0; };

// Fixed daily biology with derived clearing/build work. Dawn purchases and
// surplus sales are a deliberately simple finance baseline. Their quantities
// are commitments; actual funding still requires complete engine validation.
inline LifeDayContract compile_life_day(const std::vector<LifeProgram>& lives, const LifeAssignment& assignment,
                                        int d, const kag::Farm& farm, int land_purchases = 0, bool validate_assignment = true) {
    if ((validate_assignment && !legal_lives(lives, assignment)) || d < 0 || d >= 30) throw std::runtime_error("invalid life-day input");
    if (land_purchases < 0 || farm.n_quadrants + land_purchases > 4) throw std::runtime_error("invalid committed land purchases");
    LifeDayContract result; auto& day = result.day; auto& p = day.problem;
    std::copy_n(farm.shed, kag::N_ITEMS, p.start.shed.begin());
    std::copy_n(farm.seeds, kag::N_CROPS, p.start.seeds.begin());
    std::array<std::vector<int>, 100> chains;
    for (size_t i = 0; i < lives.size(); ++i) {
        const auto& life = lives[i]; const auto& row = life.days[d];
        if (d >= life.spec.begin_day && d <= life.release_day && (!row.work.empty() || living(row.before) || living(row.after_night))) chains[assignment[i]].push_back(i);
    }
    for (int cell = 0; cell < 100; ++cell) {
        auto state = labor::offline::managed(farm.tiles[cell / 10][cell % 10], d);
        p.start.managed_tiles.push_back({int8_t(cell % 10), int8_t(cell / 10), state});
        if (state.kind == day_solver::ManagedTileKind::LOCKED && quadrant(cell) < farm.n_quadrants + land_purchases) state = {};
        auto ending = state; auto& chain = chains[cell];
        std::stable_sort(chain.begin(), chain.end(), [&](int a, int b) { return lives[a].spec.begin_day < lives[b].spec.begin_day; });
        day_solver::TileWork work; work.tile = cell;
        for (int id : chain) {
            const auto& life = lives[id]; const auto& row = life.days[d];
            bool skip_build = false;
            if (d == life.spec.begin_day) {
                if (living(state) || state.kind == day_solver::ManagedTileKind::LOCKED) throw std::runtime_error("new life has no available tile");
                skip_build = kag::is_animal(life.spec.item) && state.kind == day_solver::structure_for(life.spec.item);
                if (!skip_build && state.kind != day_solver::ManagedTileKind::EMPTY) {
                    day_solver::TileWorkAction clear; clear.op = kag::OP_DIG; work.actions.push_back(clear); ++result.clearing_actions;
                }
            } else if (state != row.before) throw std::runtime_error("existing life differs from its biological contract");
            for (const auto& action : row.work) {
                if (skip_build && (action.op == kag::OP_BUILD_COOP || action.op == kag::OP_BUILD_PASTURE)) { ++result.reused_structures; continue; }
                work.actions.push_back(action);
            }
            state = row.after_work; ending = row.after_night;
        }
        if (chain.empty() && living(state)) throw std::runtime_error("untracked established life");
        if (!work.actions.empty()) p.tile_work.push_back(std::move(work));
        day_solver::EndTileRequirement end; end.tile = cell; end.exact_state = ending; p.required_end_tiles.push_back(end);
    }
    std::array<int64_t, kag::N_ITEMS> need{};
    std::array<int64_t, kag::N_CROPS> seeds{};
    for (const auto& work : p.tile_work) for (const auto& a : work.actions) {
        if (a.op == kag::OP_PLANT) ++seeds[a.arg];
        if (a.op == kag::OP_FEED) ++need[kag::WHEAT];
        if (a.op == kag::OP_FERTILIZE) ++need[kag::FERTILIZER];
        if (a.op == kag::OP_PLACE) ++need[a.arg];
        if (a.output_item >= 0) result.output[a.output_item] += a.output_quantity;
    }
    p.end_shed = p.start.shed; p.end_seeds = p.start.seeds;
    int slot = 0;
    auto order = [&](int op, int item, int64_t quantity) {
        if (!quantity) return;
        const int h = slot / 10, s = slot % 10; ++slot;
        if (h >= 3) throw std::runtime_error("too many dawn finance commitments");
        day.executable[h].orders[s] = {uint8_t(op), uint8_t(item), int(quantity)}; day.executable[h].n_orders = s + 1;
        if (op == kag::M_SELL) for (int t = h; t < 24; ++t) p.shed_availability[t][item] += quantity;
        else { day_solver::MarketEvent e; e.hour = h; e.order_index = s; e.market_op = op; e.item = item; e.quantity = quantity; p.market_plan.push_back(e); }
    };
    for (int item = 0; item < kag::N_PRODUCTS; ++item) {
        const auto sale = std::max<int64_t>(0, p.start.shed[item] - need[item]); order(kag::M_SELL, item, sale); p.end_shed[item] -= sale;
    }
    for (int i = 0; i < land_purchases; ++i) order(kag::M_BUY_LAND, farm.n_quadrants + i, 1);
    for (int item = 0; item < kag::N_ITEMS; ++item) if (need[item] > p.start.shed[item]) {
        const auto buy = need[item] - p.start.shed[item]; order(kag::is_animal(item) ? kag::M_BUY_ANIMAL : kag::M_BUY_PRODUCT, item, buy); p.end_shed[item] += buy;
    }
    for (int item = 0; item < kag::N_CROPS; ++item) {
        const auto buy = std::max<int64_t>(0, seeds[item] - p.start.seeds[item]); order(kag::M_BUY_SEED, item, buy); p.end_seeds[item] += buy - seeds[item];
    }
    for (int item = 0; item < kag::N_ITEMS; ++item) p.end_shed[item] += result.output[item] - need[item];
    if (d == 29) {
        int s = 0;
        for (int item = 0; item < kag::N_PRODUCTS; ++item) if (p.end_shed[item]) {
            day.executable[22].orders[s++] = {kag::M_SELL, uint8_t(item), int(p.end_shed[item])};
            for (int h = 22; h < 24; ++h) p.shed_availability[h][item] += p.end_shed[item];
            p.end_shed[item] = 0;
        }
        day.executable[22].n_orders = s;
    }
    for (auto& a : day.executable) a.finalize();
    day_scheduler::prepare_problem(p);
    if (d == 29) labor::offline::require_terminal_work(p);
    day.menu = legal_menu(day, d == 29 ? 23 : 24);
    return result;
}
}
