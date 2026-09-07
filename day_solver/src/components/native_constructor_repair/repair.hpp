#pragma once
#include "context.hpp"

namespace day_constructor {
using RepairScore = std::array<Count, 3>;
using RepairQuality = std::vector<Count>;

struct RepairOptions {
    int iterations = 20, target_routes = 0, shortlist = 128, plateau_steps = 5;
    bool strong = true, optimize_pickups = false, optimize_checkpoints = false;
    bool optimize_balance = false, separate_output_routes = true;
    double seconds = 0;
};
struct RepairStats {
    Count generated = 0, evaluated = 0;
    int visited = 0, forced_splits = 0, runs = 0;
    RepairScore best_score{};
    Count best_pickups = 0, best_checkpoints = 0;
    std::map<MoveGroup, std::pair<RepairScore, std::vector<int>>> groups;
};
struct RepairResult {
    BundlePlan routes;
    std::vector<int> lengths, workers;
    RepairScore score;
    int precedence_lateness, unmatched, impossible;
    Count unsupplied, seed_shortfall, delivery_collisions;
    // crop, hour, required, available, shortage, in reference crop/hour order.
    std::vector<std::array<Count, 5>> seed_deficits;
    RepairStats stats;
};
struct RepairStep {
    int run, step, plateau;
    const BundlePlan &current, &best;
    const RepairScore &current_score, &best_score;
    const RepairQuality &current_quality, &best_quality;
    const std::set<BundlePlan>& visited;
    Count generated, evaluated;
};
using RepairObserver = std::function<void(const RepairStep&, const NeighborhoodResult*)>;

class Repair {
    RepairContext& context_;
    RepairOptions options_;
    RepairStats stats_;
    RepairObserver observer_;
    std::optional<std::chrono::steady_clock::time_point> deadline_;

    bool expired() const { return deadline_ && std::chrono::steady_clock::now() >= *deadline_; }
    double remaining() const {
        return deadline_ ? std::max(0.0, std::chrono::duration<double>(*deadline_ - std::chrono::steady_clock::now()).count()) : -1;
    }
    std::vector<int> lengths(const BundlePlan& plan) {
        std::vector<int> result;
        for (const auto& route : plan) result.push_back(context_.length(route));
        return result;
    }
    Count item_count(const BundlePlan& plan, bool inputs) const {
        const auto& data = context_.bundles.routing;
        Count result = 0;
        for (const auto& route : plan) {
            std::set<int> items;
            for (int pattern : route) for (int id : context_.members[pattern]) {
                const auto& task = data.task_data.tasks[id];
                if (inputs && task.input >= 0) items.insert(task.input);
                else if (!inputs && data.task_data.fixed_deadline.contains(id)) items.insert(task.output);
            }
            result += items.size();
        }
        return result;
    }
    Count collisions(const BundlePlan& plan) const {
        if (!options_.separate_output_routes) return 0;
        Count result = 0;
        const auto& deadlines = context_.bundles.routing.task_data.fixed_deadline;
        for (const auto& route : plan) {
            int outputs = 0;
            for (int pattern : route) {
                bool output = false;
                for (int id : context_.members[pattern]) output |= deadlines.contains(id);
                outputs += output;
            }
            result += std::max(0, outputs - 1);
        }
        return result;
    }
    RepairScore violation(const BundlePlan& plan) {
        const auto propagation = context_.propagate(plan);
        const int unmatched = propagation.cyclic ? int(plan.size()) : options_.strong ? context_.strong(plan).unmatched :
            int(std::ranges::count(context_.matching(plan, propagation), -1));
        const auto cheap = context_.cheap(plan);
        const Count resources = propagation.lateness + unmatched + cheap[2] + cheap[1] + collisions(plan);
        return {100 * resources + cheap[6], resources, cheap[6]};
    }
    RepairQuality quality(const BundlePlan& plan, RepairScore score) {
        RepairQuality result(score.begin(), score.end());
        if (options_.optimize_checkpoints) result.push_back(item_count(plan, false));
        if (options_.optimize_pickups) result.push_back(item_count(plan, true));
        if (options_.optimize_balance) {
            const auto values = lengths(plan);
            require(!values.empty(), "cannot balance an empty route plan");
            result.push_back(*std::max_element(values.begin(), values.end()));
            result.push_back(std::accumulate(values.begin(), values.end(), Count(0)));
        } else if (result.size() > score.size()) {
            const auto values = lengths(plan);
            result.push_back(std::accumulate(values.begin(), values.end(), Count(0)));
        }
        return result;
    }
    BundlePlan strong_search(BundlePlan current) {
        auto current_score = violation(current);
        auto current_quality = quality(current, current_score);
        auto best = current;
        auto best_score = current_score;
        auto best_quality = current_quality;
        std::set<BundlePlan> visited{current};
        int plateau = 0, step = 0;
        Count generated = 0, evaluated = 0;
        std::map<MoveGroup, std::pair<RepairScore, std::vector<int>>> groups;
        const int run = ++stats_.runs;
        for (; step < options_.iterations; ++step) {
            if (expired() || (current_score[0] == 0 && !options_.optimize_pickups && !options_.optimize_checkpoints && !options_.optimize_balance)) break;
            auto sources = context_.strong(current).impossible_routes;
            if (sources.empty()) { sources.resize(current.size()); std::iota(sources.begin(), sources.end(), 0); }
            const auto selected = context_.neighbors(current, sources, visited, options_.shortlist, remaining());
            if (observer_) observer_({run, step, plateau, current, best, current_score, best_score, current_quality, best_quality, visited, generated, evaluated}, &selected);
            generated += selected.generated;
            using ChoiceKey = std::tuple<std::vector<Count>, MoveGroup, std::vector<int>, BundlePlan>;
            std::optional<ChoiceKey> choice;
            RepairScore chosen_score{};
            for (const auto& candidate : selected.selected) {
                if (expired()) break;
                const auto score = violation(candidate.plan);
                ++evaluated;
                std::vector<Count> terms(score.begin(), score.end());
                if (options_.optimize_checkpoints) terms.push_back(item_count(candidate.plan, false));
                if (options_.optimize_pickups) terms.push_back(item_count(candidate.plan, true));
                terms.insert(terms.end(), candidate.cheap.begin(), candidate.cheap.end());
                ChoiceKey key{terms, candidate.group, candidate.detail, candidate.plan};
                if (!choice || key < *choice) { choice = std::move(key); chosen_score = score; }
                const auto summary = std::pair(score, candidate.detail);
                const auto old = groups.find(candidate.group);
                if (old == groups.end() || summary < old->second) groups[candidate.group] = summary;
            }
            if (!choice) break;
            const auto& candidate = std::get<3>(*choice);
            const auto candidate_quality = quality(candidate, chosen_score);
            const bool improving = candidate_quality < current_quality;
            if (!improving && (plateau >= options_.plateau_steps || chosen_score[1] > current_score[1] + 1)) break;
            current = candidate;
            current_score = chosen_score;
            current_quality = candidate_quality;
            visited.insert(current);
            plateau = improving ? 0 : plateau + 1;
            if (current_quality < best_quality) { best = current; best_score = current_score; best_quality = current_quality; }
        }
        if (observer_) observer_({run, step, plateau, current, best, current_score, best_score, current_quality, best_quality, visited, generated, evaluated}, nullptr);
        stats_.generated = generated; stats_.evaluated = evaluated; stats_.visited = visited.size();
        stats_.best_score = best_score; stats_.best_pickups = item_count(best, true); stats_.best_checkpoints = item_count(best, false);
        stats_.groups = std::move(groups);
        return best;
    }
    std::vector<std::array<Count, 5>> seed_deficits(const BundlePlan& plan) const {
        const auto& bundles = context_.bundles;
        const auto& tasks = bundles.routing.task_data.tasks;
        std::map<int, std::array<Count, hours>> counts;
        for (int crop : context_.crop_order) counts[crop].fill(0);
        for (const auto& route : plan) {
            std::vector<int> ids;
            for (int pattern : route) ids.insert(ids.end(), context_.members[pattern].begin(), context_.members[pattern].end());
            int latest = hours - 1, successor = -1;
            for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
                const auto& task = tasks[*it];
                if (successor >= 0) latest -= 1 + distance(task.point, tasks[successor].point);
                latest = std::min(latest, bundles.late[*it]);
                if (counts.contains(task.crop)) ++counts[task.crop][std::clamp(latest, 0, hours - 1)];
                successor = *it;
            }
        }
        std::vector<std::array<Count, 5>> result;
        for (int crop : context_.crop_order) {
            Count required = 0;
            for (int hour = 0; hour < hours; ++hour) {
                required += counts[crop][hour];
                const Count available = bundles.seed_supply.at(crop)[hour];
                if (required > available) result.push_back({crop, hour, required, available, required - available});
            }
        }
        return result;
    }
public:
    Repair(RepairContext& context, RepairOptions options, RepairObserver observer = {})
        : context_(context), options_(options), observer_(std::move(observer)) {
        require(options.iterations >= 0 && options.target_routes >= 0 && options.shortlist > 0 && options.plateau_steps >= 0 && options.seconds >= 0,
                "invalid repair settings");
        require(options.strong || options.iterations == 0, "legacy non-strong repair loop not yet ported");
    }
    RepairResult run(BundlePlan routes) {
        if (options_.seconds > 0) deadline_ = std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(options_.seconds));
        std::erase_if(routes, [](const auto& route) { return route.empty(); });
        if (options_.strong && options_.iterations) {
            routes = strong_search(routes);
            while (int(routes.size()) < options_.target_routes) {
                std::optional<std::tuple<RepairScore, int, Count, int, int>> best_key;
                BundlePlan best;
                for (int source = 0; source < int(routes.size()); ++source) for (int cut = 1; cut < int(routes[source].size()); ++cut) {
                    auto candidate = routes;
                    candidate[source] = BundleRoute(routes[source].begin(), routes[source].begin() + cut);
                    candidate.emplace_back(routes[source].begin() + cut, routes[source].end());
                    const auto values = lengths(candidate);
                    const auto key = std::tuple(violation(candidate), *std::max_element(values.begin(), values.end()),
                                                std::accumulate(values.begin(), values.end(), Count(0)), source, cut);
                    if (!best_key || key < *best_key) { best_key = key; best = std::move(candidate); }
                }
                if (!best_key) break;
                routes = std::move(best);
                ++stats_.forced_splits;
            }
            if (options_.optimize_balance && !expired()) routes = strong_search(routes);
        }
        const auto score = violation(routes);
        const auto bounds = context_.propagate(routes);
        const auto matched = context_.matching(routes, bounds);
        const auto strong = bounds.cyclic ? StrongScore{int(routes.size()), int(routes.size()), {}} : options_.strong ? context_.strong(routes) :
            StrongScore{int(std::ranges::count(matched, -1)), int(std::ranges::count(matched, -1)), {}};
        const auto cheap = context_.cheap(routes);
        return {routes, lengths(routes), matched, score, bounds.lateness, strong.unmatched, strong.impossible,
                cheap[2], cheap[1], collisions(routes), seed_deficits(routes), stats_};
    }
};
}  // namespace day_constructor
