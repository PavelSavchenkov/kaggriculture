#pragma once
#include "life_search.hpp"
#include "route_repair.hpp"
#include "certify.hpp"

namespace placement {
// Identical lives are interchangeable. Keep common occupied cells paired
// before mapping the remaining lives, avoiding artificial identity swaps.
inline LifeAssignment align_lives(const std::vector<LifeProgram>& lives, const LifeAssignment& reference, LifeAssignment assignment) {
    const LifeEquivalence equivalence(lives);
    for (const auto& ids : equivalence.groups) {
        std::vector<int> remaining; for (int id : ids) remaining.push_back(assignment[id]);
        std::vector<int> unmatched;
        for (int id : ids) {
            const auto found = std::find(remaining.begin(), remaining.end(), reference[id]);
            if (found == remaining.end()) unmatched.push_back(id);
            else { assignment[id] = *found; remaining.erase(found); }
        }
        for (int id : unmatched) {
            auto distance = [&](int cell) { return std::abs(cell % 10 - reference[id] % 10) + std::abs(cell / 10 - reference[id] / 10); };
            const auto closest = std::min_element(remaining.begin(), remaining.end(), [&](int a, int b) { return std::pair{distance(a), a} < std::pair{distance(b), b}; });
            assignment[id] = *closest; remaining.erase(closest);
        }
    }
    return assignment;
}

inline std::optional<Anchors> life_anchors(const std::vector<LifeProgram>& lives, const LifeAssignment& reference,
                                           const LifeAssignment& assignment, int d, const Day& source, const Day& target) {
    struct Task { day_solver::TileWorkAction action; int target; };
    std::array<std::vector<Task>, 100> from, to;
    std::vector<int> order(lives.size()); std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return lives[a].spec.begin_day < lives[b].spec.begin_day; });
    for (int id : order) for (const auto& action : lives[id].days[d].work) {
        from[reference[id]].push_back({action, assignment[id]});
        to[assignment[id]].push_back({action, assignment[id]});
    }
    auto matches = [](const DayProblem& problem, const auto& tasks) {
        std::array<bool, 100> seen{};
        for (const auto& tile : problem.tile_work) {
            const auto& physical = problem.start.managed_tiles[tile.tile]; const int cell = physical.y * 10 + physical.x;
            if (tasks[cell].size() != tile.actions.size()) return false;
            seen[cell] = true;
            for (size_t i = 0; i < tile.actions.size(); ++i) {
                const auto& a = tile.actions[i]; const auto& b = tasks[cell][i].action;
                if (a.op != b.op || a.arg != b.arg || a.output_item != b.output_item || a.output_quantity != b.output_quantity) return false;
            }
        }
        for (int cell = 0; cell < 100; ++cell) if (!seen[cell] && !tasks[cell].empty()) return false;
        return true;
    };
    // Weed clearing and structure reuse that change the task sequence are
    // outside this bounded repair; the normal compiler can solve them.
    if (!matches(source.problem, from) || !matches(target.problem, to)) return std::nullopt;
    auto anchors = source_anchors(source, identity());
    std::array<size_t, 40> cursor{}; std::array<size_t, 100> tasks{};
    for (int h = 0; h < 24; ++h) for (int u = 0; u < 40; ++u) {
        if (cursor[u] >= anchors[u].size() || anchors[u][cursor[u]].hour != h) continue;
        auto& anchor = anchors[u][cursor[u]++]; const int cell = anchor.cell;
        const bool place = anchor.action.op == kag::OP_PLACE && kag::is_animal(anchor.action.arg) && tasks[cell] < from[cell].size() && from[cell][tasks[cell]].action.op == kag::OP_PLACE;
        const bool field = place || (anchor.action.op >= kag::OP_PLANT && anchor.action.op <= kag::OP_CARE);
        if (!field) continue;
        if (tasks[cell] >= from[cell].size() || from[cell][tasks[cell]].action.op != anchor.action.op) throw std::runtime_error("life anchors disagree with certified source");
        anchor.cell = from[cell][tasks[cell]++].target;
    }
    return anchors;
}

class LifeRouteSource {
    PlacementProgram program;
    LifeAssignment reference;
    Course course;
public:
    int attempts = 0, matches = 0;
    explicit LifeRouteSource(const fs::path& folder) : program(load_placement_program((folder / "INPUT.plan").string())),
        reference(read_life_assignment(folder / "assignment.txt", program.lives.size())), course(folder) {}
    void validate(const PlacementProgram& target) const {
        if (program.land != target.land || program.lives.size() != target.lives.size()) throw std::runtime_error("route source has a different life program");
        for (size_t i = 0; i < program.lives.size(); ++i) if (program.lives[i].spec != target.lives[i].spec) throw std::runtime_error("route source has different life specifications");
    }
    Certificate find(const Day& day, int d, const LifeAssignment& assignment, int maximum_workers = 40) {
        Certificate result;
        const auto& source = course.days[d]; const int workers = source.problem.worker_count;
        if (workers > maximum_workers) return result;
        auto problem = day.problem; problem.worker_count = workers;
        std::erase_if(problem.market_plan, [](const auto& event) { return event.market_op == kag::M_HIRE; });
        for (const auto& event : source.problem.market_plan) if (event.market_op == kag::M_HIRE) {
            const auto op = day.executable[event.hour].orders[event.order_index].op;
            if (op != kag::M_NONE && op != kag::M_HIRE) return result;
            problem.market_plan.push_back(event);
        }
        std::sort(problem.market_plan.begin(), problem.market_plan.end(), [](const auto& a, const auto& b) { return std::pair{a.hour, a.order_index} < std::pair{b.hour, b.order_index}; });
        day_scheduler::prepare_problem(problem);
        if (d == 29) labor::offline::require_terminal_work(problem);
        auto direct = source.physical; labor::offline::physical_orders(problem, direct);
        const auto replay = day_solver::replay_schedule(problem, direct); ++attempts;
        if (replay.candidate.replay.strict_valid && replay.requirements_satisfied && replay.invariants_satisfied && replay.errors.empty()) {
            result.workers = workers; result.problem = std::move(problem); result.schedule = direct; result.initial_witness = "lifetime source schedule";
            ++matches; return result;
        }
        const auto aligned = align_lives(program.lives, reference, assignment);
        const auto anchors = life_anchors(program.lives, reference, aligned, d, source, day);
        if (!anchors) return result;
        for (int variant = 0; variant < 4; ++variant) {
            ++attempts; const auto repaired = route_repair_anchors(source, problem, *anchors, variant);
            if (!repaired) continue;
            result.workers = workers; result.problem = std::move(problem); result.schedule = repaired; result.initial_witness = "lifetime route repair";
            ++matches; return result;
        }
        return result;
    }
};
}
