#pragma once
#include "cold_cases.hpp"
#include "search.hpp"
#include "fixed_finance.hpp"

namespace placement {
inline kag::Tile physical_tile(const day_solver::ManagedTileState& state, int day) {
    using K = day_solver::ManagedTileKind;
    kag::Tile tile;
    switch (state.kind) {
        case K::EMPTY: tile.kind = kag::T_EMPTY; break;
        case K::LOCKED: tile.kind = kag::T_LOCKED; break;
        case K::WEED: tile.kind = kag::T_WEED; break;
        case K::CROP: tile.kind = kag::T_PLANT; break;
        case K::COOP: tile.kind = kag::T_COOP; break;
        case K::PASTURE: tile.kind = kag::T_PASTURE; break;
    }
    tile.what = state.animal >= 0 ? state.animal : state.crop >= 0 ? state.crop : 0;
    tile.has_animal = state.animal >= 0; tile.planted_day = day - state.age_days;
    tile.yield_units = state.stored_units; tile.consecutive_dry = state.consecutive_dry_days;
    tile.pending_care_bonus = state.pending_care_bonus; tile.fertilized_until_day = day + state.fertilizer_days_remaining - 1;
    tile.watered_today = state.watered_today; tile.fed_today = state.fed_today; tile.cared_today = state.cared_today;
    tile.fertilizer_available = state.fertilizer_available;
    return tile;
}

struct AssignmentHash {
    size_t operator()(const LifeAssignment& assignment) const {
        uint64_t hash = 1469598103934665603ULL;
        for (int cell : assignment) { hash ^= cell; hash *= 1099511628211ULL; }
        return hash;
    }
};
struct LifeEquivalence {
    std::vector<std::vector<int>> groups;
    std::vector<int> group;
    explicit LifeEquivalence(const std::vector<LifeProgram>& lives) : group(lives.size()) {
        for (size_t i = 0; i < lives.size(); ++i) {
            size_t found = 0;
            while (found < groups.size() && lives[groups[found][0]].spec != lives[i].spec) ++found;
            if (found == groups.size()) groups.emplace_back();
            groups[found].push_back(i); group[i] = found;
        }
    }
    void normalize(LifeAssignment& assignment) const {
        std::vector<int> cells;
        for (const auto& ids : groups) if (ids.size() > 1) {
            cells.clear(); for (int id : ids) cells.push_back(assignment[id]);
            std::sort(cells.begin(), cells.end());
            for (size_t i = 0; i < ids.size(); ++i) assignment[ids[i]] = cells[i];
        }
    }
};
class LifeEvaluator {
    const std::vector<LifeProgram>& lives;
    std::array<int, 30> land;
    LifeEquivalence equivalence;
    bool collapse_identical;
    bool reuse_days, repeat_validation;
    const std::array<Day, 30>* fixed_finance;
    std::unordered_map<LifeAssignment, Score, AssignmentHash> cache;
    struct CachedDay {
        double cost, workers;
        int lower, quadrants;
        bool rejected;
        std::array<kag::Tile, 100> tiles;
        std::array<kag::Count, kag::N_ITEMS> shed;
        std::array<kag::Count, kag::N_CROPS> seeds;
    };
    std::array<std::vector<int>, 30> active;
    std::array<std::unordered_map<std::string, CachedDay>, 30> day_cache;
    size_t day_entries = 0;
    // One immutable program/day per cache. Include every farm field read by
    // compile_life_day and every active life location. Future/past locations
    // are irrelevant after the complete assignment's legality was checked.
    std::string day_key(const kag::Farm& farm, const LifeAssignment& assignment, int d) const {
        std::string key; key.reserve(1900 + active[d].size());
        auto append = [&](uint32_t value, int bytes) { for (int i = 0; i < bytes; ++i) key.push_back(char((value >> (8 * i)) & 255)); };
        append(farm.n_quadrants, 4);
        for (int value : farm.shed) append(value, 4);
        for (int value : farm.seeds) append(value, 4);
        for (const auto& row : farm.tiles) for (const auto& tile : row) {
            append(tile.kind, 1); append(tile.what, 1); append(tile.has_animal, 1);
            append(tile.watered_today, 1); append(tile.fed_today, 1); append(tile.cared_today, 1); append(tile.fertilizer_available, 1);
            append(uint8_t(tile.consecutive_dry), 1); append(uint8_t(tile.yield_units), 1); append(uint8_t(tile.pending_care_bonus), 1);
            append(uint16_t(tile.planted_day), 2); append(uint16_t(tile.fertilized_until_day), 2); append(tile.max_lifespan_step, 4);
        }
        for (int id : active[d]) append(assignment[id], 1);
        return key;
    }
public:
    int queries = 0, hits = 0, day_queries = 0, day_hits = 0, day_evictions = 0;
    explicit LifeEvaluator(const std::vector<LifeProgram>& input, std::array<int, 30> land_plan = {}, bool collapse = true,
                           bool reuse = true, bool revalidate = false, const std::array<Day, 30>* finance = nullptr) :
        lives(input), land(land_plan), equivalence(input), collapse_identical(collapse), reuse_days(reuse), repeat_validation(revalidate), fixed_finance(finance) {
        for (int d = 0; d < 30; ++d) for (size_t i = 0; i < lives.size(); ++i) {
            const auto& life = lives[i]; const auto& row = life.days[d];
            if (d >= life.spec.begin_day && d <= life.release_day && (!row.work.empty() || living(row.before) || living(row.after_night))) active[d].push_back(i);
        }
    }
    Score operator()(LifeAssignment assignment) {
        if (collapse_identical) equivalence.normalize(assignment);
        if (const auto found = cache.find(assignment); found != cache.end()) { ++hits; return found->second; }
        if (!legal_lives(lives, assignment)) throw std::runtime_error("illegal life placement");
        kag::Config config; config.weed_chance = 0; auto farm = kag::Sim(config).st.farms[0];
        Score score;
        for (int d = 0; d < 30; ++d) {
            CachedDay value; const CachedDay* used = &value;
            auto key = reuse_days ? day_key(farm, assignment, d) : std::string{};
            const auto found = reuse_days ? day_cache[d].find(key) : day_cache[d].end();
            if (found != day_cache[d].end()) { used = &found->second; ++day_hits; }
            else {
                auto contract = compile_life_day(lives, assignment, d, farm, land[d], repeat_validation);
                if (fixed_finance) apply_fixed_finance(contract, (*fixed_finance)[d], d);
                const auto& problem = contract.day.problem;
                const auto estimate = fast_day_solver_estimator::estimate_day(problem, contract.day.menu); ++day_queries;
                value.cost = estimate.analytically_rejected ? 1e12 : estimate.cost;
                value.workers = estimate.analytically_rejected ? 41 : estimate.workers;
                value.lower = estimate.lower; value.rejected = estimate.analytically_rejected; value.quadrants = farm.n_quadrants + land[d];
                for (int item = 0; item < kag::N_ITEMS; ++item) value.shed[item] = problem.end_shed[item];
                for (int item = 0; item < kag::N_CROPS; ++item) value.seeds[item] = problem.end_seeds[item];
                for (const auto& end : problem.required_end_tiles) value.tiles[end.tile] = physical_tile(*end.exact_state, d + 1);
                if (reuse_days) {
                    if (day_entries >= 10000) { for (auto& cache : day_cache) cache.clear(); day_entries = 0; ++day_evictions; }
                    used = &day_cache[d].emplace(std::move(key), value).first->second; ++day_entries;
                }
            }
            score.day_cost[d] = used->cost; score.day_workers[d] = used->workers;
            score.lower[d] = used->lower; score.rejected += used->rejected;
            score.cost += score.day_cost[d]; score.workers += score.day_workers[d];
            // Only an unbounded, weed-free physical projection for ranking.
            // Exact compilation starts each day from the actual own farm.
            std::copy(used->shed.begin(), used->shed.end(), farm.shed); std::copy(used->seeds.begin(), used->seeds.end(), farm.seeds);
            for (int cell = 0; cell < 100; ++cell) farm.tiles[cell / 10][cell % 10] = used->tiles[cell];
            farm.n_quadrants = used->quadrants;
        }
        ++queries;
        if (cache.size() >= 5000) cache.clear();
        cache.emplace(assignment, score);
        return score;
    }
};

struct LifeCandidate { std::string method; LifeAssignment assignment; Score score; int passes = 0; bool exhausted = false; };
inline LifeCandidate search_lives(const std::vector<LifeProgram>& lives, LifeEvaluator& evaluate, LifeCandidate initial,
                                  double seconds, uint64_t seed, bool equal_radius) {
    auto best = initial, current = initial;
    const LifeEquivalence equivalence(lives);
    equivalence.normalize(current.assignment); best.assignment = current.assignment;
    struct Move { int first, second; bool swap; };
    std::vector<Move> moves;
    for (int first = 0; first < int(lives.size()); ++first) {
        for (int cell = 0; cell < 100; ++cell) if (lives[first].spec.allowed[cell]) moves.push_back({first, cell, false});
        for (int second = first + 1; second < int(lives.size()); ++second)
            if (equivalence.group[first] != equivalence.group[second]) moves.push_back({first, second, true});
    }
    std::mt19937_64 rng(seed);
    const auto began = std::chrono::steady_clock::now();
    bool expired = false;
    int passes = 0;
    while (!expired) {
        bool improved = false; ++passes;
        std::shuffle(moves.begin(), moves.end(), rng);
        for (const auto& move : moves) {
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count() >= seconds) { expired = true; break; }
            auto assignment = current.assignment;
            const int destination = move.swap ? assignment[move.second] : move.second;
            if (destination == assignment[move.first] || (equal_radius && labor::shed_distance(destination) != labor::shed_distance(assignment[move.first]))) continue;
            if (move.swap) std::swap(assignment[move.first], assignment[move.second]);
            else assignment[move.first] = destination;
            equivalence.normalize(assignment);
            if (assignment == current.assignment || !legal_lives(lives, assignment)) continue;
            const auto score = evaluate(assignment);
            if (std::pair{score.cost, score.workers} < std::pair{current.score.cost, current.score.workers}) {
                current = {"search", assignment, score}; improved = true;
            }
        }
        best = current;
        if (!improved && !expired) { best.exhausted = true; break; }
    }
    best.passes = passes;
    best.method = equal_radius ? "equal_radius_lives" : "local_lives";
    return best;
}
}
