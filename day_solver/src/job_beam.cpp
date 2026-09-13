#include "job_beam.hpp"
#include "components/native_constructor_data/task_data.hpp"
#include "components/native_constructor_data/vendor/rectangular_lsap.h"
#include "components/native_materialized_screen/screen.hpp"
#include "components/native_solver_api/exact.hpp"
#include "storage.hpp"
#include "dispatch_weights.hpp"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model_solver.h"
#include <unordered_set>
#include <unordered_map>
#include <limits>
#include <random>

namespace day_scheduler::beam_detail {
namespace dc = day_constructor;
namespace dn = day_native;
using Stock = std::array<int, kag::N_ITEMS>;
using Route = std::vector<int>;
struct Job { std::vector<int> tasks; dc::Point point; int due = 24, predecessor = -1; };
struct ReturnOffer { Stock cargo{}; int hour = 0, delay = 0, travel = 0, rank = 0, service = 1; bool retaining = false; };
struct RouteScore {
    Stock inputs{}, returned{}, cargo{}, produced{};
    Stock pickup_hour{};
    std::vector<ReturnOffer> offers;
    std::vector<std::pair<int, int>> spans;
    std::vector<std::pair<int, int>> seed_deadlines;
    std::vector<std::array<int, 3>> input_deadlines;
    dc::Point end{};
    int length = 0, late = 0, travel = 0;
    double cost = 0;
};
struct State {
    std::vector<Route> routes;
    std::vector<RouteScore> route_scores;
    Stock inputs{};
    double route_cost = 0, cost = 0;
};
struct CachedRoute { int worker; Route route; RouteScore score; };

struct Search {
    dc::TaskData data;
    std::vector<Job> jobs;
    std::vector<dc::Point> starts;
    std::vector<int> releases;
    Stock supply{}, ready{}, production{};
    bool split_jobs = false;
    int delivery_score;
    bool alternate_purchase_scores;
    bool return_dp;
    bool cache_routes = false;
    mutable std::unordered_map<uint64_t, std::vector<CachedRoute>> route_cache;
    mutable int route_cache_hits = 0, route_cache_misses = 0, route_cache_size = 0, route_cache_resets = 0;
    std::map<int, std::array<int, 24>> seed_supply;
    std::map<int, std::array<int, 24>> input_supply;
    int generated = 0, depth = 0;
    int dispatch_rollouts = 0, dispatch_mixed = 0;
    explicit Search(const day_solver::DayProblem& problem, int max_job_tasks, int delivery_score = 0, bool urgent_fragments = false, bool seed_deadlines = false, bool pickup_deadlines = false, bool alternate_purchase_scores = false, bool return_dp = false)
        : data(problem, false), delivery_score(delivery_score), alternate_purchase_scores(alternate_purchase_scores), return_dp(return_dp) {
        if (seed_deadlines) for (int crop = 0; crop < kag::N_CROPS; ++crop) {
            const int plants = std::ranges::count_if(data.tasks, [&](const auto& task) { return task.crop == crop; });
            if (problem.start.seeds[crop] >= plants) continue;
            auto& available = seed_supply[crop];
            available.fill(int(problem.start.seeds[crop]));
            for (const auto& order : problem.market_plan)
                if (order.market_op == kag::M_BUY_SEED && order.item == crop)
                    for (int hour = order.hour + 1; hour < 24; ++hour)
                        available[hour] = int(std::min<dc::Count>(plants, dc::Count(available[hour]) + order.quantity));
        }
        if (pickup_deadlines) for (int item = 0; item < kag::N_ITEMS; ++item) {
            const int needed = std::ranges::count_if(data.tasks, [&](const auto& task) { return task.input == item; });
            if (problem.start.shed[item] >= needed ||
                std::ranges::any_of(data.tasks, [&](const auto& task) { return task.output == item; })) continue;
            auto& available = input_supply[item];
            available.fill(int(problem.start.shed[item]));
            for (const auto& order : problem.market_plan)
                if ((order.market_op == kag::M_BUY_PRODUCT || order.market_op == kag::M_BUY_ANIMAL) && order.item == item)
                    for (int hour = order.hour + 1; hour < 24; ++hour)
                        available[hour] = int(std::min<dc::Count>(needed, dc::Count(available[hour]) + order.quantity));
        }
        // A harvest followed by feeding on its own tile is a poor default
        // sale producer when an equally large independent harvest is available.
        // Rematch only interchangeable producers; timed completion keeps the
        // original aggregate deadlines and quantities unchanged.
        auto local_consumers = [&](int id) {
            int consumers = 0;
            const auto& producer = data.tasks[id];
            for (int next = id + 1; next < int(data.tasks.size()) &&
                 data.tasks[next].pattern == producer.pattern; ++next) {
                if (data.tasks[next].output == producer.output) break;
                consumers += data.tasks[next].input == producer.output;
            }
            return consumers;
        };
        std::vector<std::pair<int, int>> consumed_producers;
        for (const auto& [task, hour] : data.fixed_deadline)
            if (hour < 23 && local_consumers(task)) consumed_producers.push_back({task, hour});
        if (!consumed_producers.empty()) {
            const auto earliest = data.delivery_earliest();
            for (const auto& [selected, hour] : consumed_producers) {
                const auto& producer = data.tasks[selected];
                int alternative = -1;
                for (const auto& task : data.tasks) {
                    if (task.output != producer.output || task.quantity < producer.quantity ||
                        data.fixed_deadline.contains(task.id) || earliest[task.id] > hour || local_consumers(task.id)) continue;
                    if (alternative < 0 || std::pair(data.delivery_estimate(task.id), task.id) <
                        std::pair(data.delivery_estimate(alternative), alternative)) alternative = task.id;
                }
                if (alternative >= 0) {
                    data.fixed_deadline.erase(selected);
                    data.fixed_deadline[alternative] = hour;
                }
            }
        }
        for (const auto& task : data.tasks) {
            if (task.output >= 0) production[task.output] += task.quantity;
            const bool split = task.predecessor < 0 || (max_job_tasks && int(jobs.back().tasks.size()) >= max_job_tasks) ||
                (data.fixed_deadline.contains(task.predecessor) &&
                data.fixed_deadline.at(task.predecessor) < 23);
            if (split) {
                const int predecessor = task.predecessor < 0 ? -1 : int(jobs.size()) - 1;
                jobs.push_back({{}, task.point, 24, predecessor});
                split_jobs |= predecessor >= 0;
            }
            jobs.back().tasks.push_back(task.id);
            if (data.fixed_deadline.contains(task.id)) jobs.back().due = std::min(jobs.back().due, data.fixed_deadline.at(task.id));
        }
        // A producer's deadline also makes the preceding work on its tile
        // urgent. Otherwise fine-grained insertion postpones the prerequisites
        // while considering their output as an early-priority job.
        if (max_job_tasks) for (int job = int(jobs.size()) - 1; job >= 0; --job)
            if (jobs[job].predecessor >= 0)
                jobs[jobs[job].predecessor].due = std::min(jobs[jobs[job].predecessor].due, jobs[job].due);
        if (delivery_score) data.fixed_deadline.clear();
        std::array<int, 4> occupancy{1, 0, 0, 0};
        for (int worker = 0; worker < problem.worker_count; ++worker) {
            const int access = worker ? int(std::min_element(occupancy.begin(), occupancy.end()) - occupancy.begin()) : 0;
            if (worker) ++occupancy[access];
            starts.push_back(dc::shed[access]); releases.push_back(worker ? data.hires[worker - 1] + 1 : 0);
        }
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            // A ranking penalty for scarce initial inputs, not a feasibility
            // constraint: later production and purchases can replenish stock.
            supply[item] = std::clamp<dc::Count>(problem.start.shed[item] + data.bought(item, 3) -
                problem.shed_availability[2][item], 0, data.tasks.size());
            ready[item] = 0;
            if (problem.start.shed[item]) continue;
            int first = 24;
            for (const auto& order : problem.market_plan)
                if ((order.market_op == kag::M_BUY_PRODUCT || order.market_op == kag::M_BUY_ANIMAL) && order.item == item)
                    first = std::min(first, int(order.hour) + 1);
            ready[item] = first < 24 ? first : 0;
        }
        if (urgent_fragments) {
            std::vector<Job> expanded;
            std::vector<int> last(jobs.size());
            for (int index = 0; index < int(jobs.size()); ++index) {
                const auto& job = jobs[index];
                int eligible = 0;
                if (job.due < 23 && job.tasks.size() > 1) {
                    Stock balance{}, needed{};
                    for (int id : job.tasks) {
                        const auto& task = data.tasks[id];
                        if (task.input >= 0) {
                            --balance[task.input];
                            needed[task.input] = std::max(needed[task.input], -balance[task.input]);
                        }
                        if (task.output >= 0) balance[task.output] += task.quantity;
                    }
                    for (int worker = 0; worker < int(starts.size()); ++worker) {
                        int time = releases[worker];
                        for (int item = 0; item < kag::N_ITEMS; ++item)
                            if (needed[item]) time = std::max(time, ready[item]) + 1;
                        time += dc::distance(starts[worker], job.point);
                        for (int id : job.tasks) time = std::max(time, data.early[id]) + 1;
                        eligible += time + dc::tail(job.point) <= job.due;
                    }
                } else eligible = 2;
                int previous = job.predecessor >= 0 ? last[job.predecessor] : -1;
                if (eligible <= 1) {
                    for (int id : job.tasks) {
                        expanded.push_back({{id}, job.point, job.due, previous});
                        previous = int(expanded.size()) - 1;
                    }
                    split_jobs = true;
                } else expanded.push_back({job.tasks, job.point, job.due, previous});
                last[index] = int(expanded.size()) - 1;
            }
            jobs = std::move(expanded);
            for (int job = int(jobs.size()) - 1; job >= 0; --job)
                if (jobs[job].predecessor >= 0)
                    jobs[jobs[job].predecessor].due = std::min(jobs[jobs[job].predecessor].due, jobs[job].due);
        }
    }

    RouteScore improve_returns(const Route& route, int worker, const RouteScore& original) const {
        // Labels choose depot visits between whole jobs. Returning all surplus
        // leaves exactly the remaining input requirement, so cargo at a later
        // prefix depends only on the last return prefix, not earlier visits.
        // This is a route score; the timing model still chooses actual returns.
        const int count = route.size();
        std::vector<Stock> net(count + 1), reserved(count + 1);
        int initial_time = releases[worker];
        for (int item = 0; item < kag::N_ITEMS; ++item)
            if (original.inputs[item]) initial_time = std::max(initial_time, ready[item]) + 1;
        int earliest = initial_time;
        auto point = starts[worker];
        for (int rank = 0; rank < count; ++rank) {
            const auto& job = jobs[route[rank]];
            earliest += dc::distance(point, job.point); point = job.point;
            net[rank + 1] = net[rank];
            for (int id : job.tasks) {
                earliest = std::max(earliest, data.early[id]);
                // Visiting a shed cannot shorten travel between fixed jobs.
                if (earliest > data.late[id] || earliest >= 24) return original;
                ++earliest;
                const auto& task = data.tasks[id];
                if (task.input >= 0) --net[rank + 1][task.input];
                if (task.output >= 0) net[rank + 1][task.output] += task.quantity;
            }
        }
        for (int rank = count - 1; rank >= 0; --rank) {
            reserved[rank] = reserved[rank + 1];
            const auto& tasks = jobs[route[rank]].tasks;
            for (auto it = tasks.rbegin(); it != tasks.rend(); ++it) {
                const auto& task = data.tasks[*it];
                if (task.output >= 0) reserved[rank][task.output] = std::max(0, reserved[rank][task.output] - int(task.quantity));
                if (task.input >= 0) ++reserved[rank][task.input];
            }
        }
        struct Label {
            int time, travel;
            dc::Point point;
            std::vector<std::pair<int, int>> spans;
        };
        std::vector<std::vector<Label>> labels(count + 1);
        labels[0].push_back({initial_time, 0, starts[worker], {}});
        RouteScore best = original;
        auto finish = [&](const Label& label, int returned_prefix) {
            const double cost = 4.0 * label.travel + 0.2 * (label.time - releases[worker]) * (label.time - releases[worker]);
            if (cost >= best.cost) return;
            best = original; best.cost = cost; best.length = label.time;
            best.travel = label.travel; best.late = 0; best.end = label.point; best.spans = label.spans;
            for (int item = 0; item < kag::N_ITEMS; ++item) {
                best.returned[item] = original.inputs[item] + net[returned_prefix][item] - reserved[returned_prefix][item];
                best.cargo[item] = original.inputs[item] + net[count][item] - best.returned[item];
            }
        };
        for (int first = 0; first < count; ++first) for (const auto& label : labels[first]) {
            auto current = label;
            Stock due; due.fill(24);
            for (int rank = first; rank < count; ++rank) {
                const auto& job = jobs[route[rank]];
                const int travel = dc::distance(current.point, job.point);
                current.travel += travel; current.time += travel; current.point = job.point;
                const int begin = std::max(current.time, data.early[job.tasks.front()]);
                bool late = false;
                for (int id : job.tasks) {
                    current.time = std::max(current.time, data.early[id]);
                    late |= current.time > data.late[id] || current.time >= 24;
                    const auto& task = data.tasks[id];
                    if (task.output >= 0 && data.fixed_deadline.contains(id))
                        due[task.output] = std::min(due[task.output], data.fixed_deadline.at(id));
                    ++current.time;
                }
                if (late) break;
                current.spans.push_back({begin, current.time});
                const int next = rank + 1;
                if (next == count && std::ranges::all_of(due, [](int hour) { return hour >= 23; }))
                    finish(current, first);
                std::vector<std::pair<int, int>> deposits;
                bool retaining = false, missing = false;
                for (int item = 0; item < kag::N_ITEMS; ++item) {
                    const int surplus = reserved[first][item] + net[next][item] - net[first][item] - reserved[next][item];
                    if (surplus > 0) deposits.push_back({due[item], item});
                    else missing |= due[item] < 23;
                    retaining |= reserved[next][item] > 0;
                }
                if (missing || deposits.empty() || std::ranges::all_of(due, [](int hour) { return hour >= 23; })) continue;
                std::sort(deposits.begin(), deposits.end());
                for (auto home : dc::shed) {
                    auto candidate = current;
                    const int back = dc::distance(candidate.point, home), arrival = candidate.time + back;
                    candidate.time = arrival + (retaining ? int(deposits.size()) : 1);
                    if (candidate.time > 24) continue;
                    bool missed = false;
                    for (int index = 0; index < int(deposits.size()); ++index)
                        missed |= deposits[index].first < 23 && arrival + (retaining ? index : 0) > deposits[index].first;
                    if (missed) continue;
                    candidate.travel += back; candidate.point = home;
                    if (next == count) { finish(candidate, next); continue; }
                    auto& frontier = labels[next];
                    if (std::ranges::any_of(frontier, [&](const auto& old) {
                        return old.point == home && old.time <= candidate.time && old.travel <= candidate.travel;
                    })) continue;
                    std::erase_if(frontier, [&](const auto& old) {
                        return old.point == home && old.time >= candidate.time && old.travel >= candidate.travel;
                    });
                    frontier.push_back(std::move(candidate));
                }
            }
        }
        return best;
    }

    RouteScore evaluate(const Route& route, int worker) const {
        if (!cache_routes || route.empty()) return evaluate_route(route, worker);
        uint64_t key = (14695981039346656037ULL ^ uint64_t(worker)) * 1099511628211ULL;
        for (int job : route) key = (key ^ uint64_t(job + 1)) * 1099511628211ULL;
        const auto found = route_cache.find(key);
        if (found != route_cache.end()) for (const auto& cached : found->second)
            if (cached.worker == worker && cached.route == route) {
                ++route_cache_hits;
                return cached.score;
            }
        ++route_cache_misses;
        const auto result = evaluate_route(route, worker);
        if (route_cache_size == 16384) {
            route_cache.clear(); route_cache_size = 0; ++route_cache_resets;
        }
        route_cache[key].push_back({worker, route, result});
        ++route_cache_size;
        return result;
    }

    RouteScore evaluate_route(const Route& route, int worker) const {
        RouteScore result;
        if (route.empty()) return result;
        Stock balance{};
        std::vector<int> tasks;
        for (int job : route) for (int id : jobs[job].tasks) {
            tasks.push_back(id);
            const auto& task = data.tasks[id];
            if (task.input >= 0) {
                --balance[task.input];
                result.inputs[task.input] = std::max(result.inputs[task.input], -balance[task.input]);
            }
            if (task.output >= 0) {
                balance[task.output] += task.quantity; result.produced[task.output] += task.quantity;
            }
        }
        if (!seed_supply.empty() || !input_supply.empty()) {
            int latest = 23, successor = -1;
            Stock pickup_latest;
            pickup_latest.fill(23);
            for (auto it = tasks.rbegin(); it != tasks.rend(); ++it) {
                const auto& task = data.tasks[*it];
                if (successor >= 0) latest -= 1 + dc::distance(task.point, data.tasks[successor].point);
                latest = std::min(latest, data.late[*it]);
                if (seed_supply.contains(task.crop))
                    result.seed_deadlines.push_back({task.crop, std::clamp(latest, 0, 23)});
                if (input_supply.contains(task.input))
                    pickup_latest[task.input] = std::min(pickup_latest[task.input], latest - dc::tail(task.point) - 1);
                successor = *it;
            }
            for (const auto& [item, available] : input_supply)
                if (result.inputs[item]) result.input_deadlines.push_back({item, std::clamp(pickup_latest[item], 0, 23), result.inputs[item]});
        }
        int time = releases[worker], pending = 24;
        for (int item = 0; item < kag::N_ITEMS; ++item) if (result.inputs[item]) {
            time = std::max(time, ready[item]); result.pickup_hour[item] = time++;
        }
        std::vector<Stock> reserved;
        if (delivery_score >= 2) {
            reserved.resize(tasks.size() + 1);
            for (int position = int(tasks.size()) - 1; position >= 0; --position) {
                reserved[position] = reserved[position + 1];
                const auto& task = data.tasks[tasks[position]];
                if (task.output >= 0) reserved[position][task.output] = std::max(0, reserved[position][task.output] - int(task.quantity));
                if (task.input >= 0) ++reserved[position][task.input];
            }
        }
        auto point = starts[worker];
        Stock cargo = result.inputs;
        int done = 0;
        auto deposit = [&] {
            const auto home = dc::nearest_shed(point);
            result.travel += dc::distance(point, home);
            time += dc::distance(point, home);
            result.late += std::max(0, time - pending);
            Stock reserved{};
            for (int position = int(tasks.size()) - 1; position >= done; --position) {
                const auto& task = data.tasks[tasks[position]];
                if (task.output >= 0) reserved[task.output] = std::max(0, reserved[task.output] - int(task.quantity));
                if (task.input >= 0) ++reserved[task.input];
            }
            int types = 0; bool retaining = false;
            for (int item = 0; item < kag::N_ITEMS; ++item) {
                const int amount = std::max(0, cargo[item] - reserved[item]);
                result.returned[item] += amount; cargo[item] -= amount;
                types += amount > 0; retaining |= cargo[item] > 0;
            }
            time += retaining ? std::max(1, types) : 1;
            point = home; pending = 24;
        };
        for (int rank = 0; rank < int(route.size()); ++rank) {
            const int job = route[rank];
            const auto& next = jobs[job];
            if (pending < 24 && time + dc::distance(point, next.point) + int(next.tasks.size()) + dc::tail(next.point) > pending)
                deposit();
            result.travel += dc::distance(point, next.point);
            time += dc::distance(point, next.point); point = next.point;
            const int first = std::max(time, data.early[next.tasks.front()]);
            for (int id : next.tasks) {
                time = std::max(time, data.early[id]);
                result.late += std::max(0, time - data.late[id]);
                if (data.fixed_deadline.contains(id) && data.fixed_deadline.at(id) < 23)
                    pending = std::min(pending, data.fixed_deadline.at(id));
                if (data.tasks[id].input >= 0) --cargo[data.tasks[id].input];
                if (data.tasks[id].output >= 0) cargo[data.tasks[id].output] += data.tasks[id].quantity;
                ++time; ++done;
            }
            result.spans.push_back({first, time});
            if (delivery_score >= 2) {
                Stock available{};
                int types = 0; bool retaining = false;
                for (int item = 0; item < kag::N_ITEMS; ++item) {
                    available[item] = std::max(0, cargo[item] - reserved[done][item]);
                    types += available[item] > 0; retaining |= cargo[item] > available[item];
                }
                if (types) for (auto access : dc::shed) {
                    const int back = dc::distance(point, access), service = retaining ? types : 1;
                    const int travel = back + (rank + 1 < int(route.size()) ?
                        dc::distance(access, jobs[route[rank + 1]].point) - dc::distance(point, jobs[route[rank + 1]].point) : 0);
                    result.offers.push_back({available, time + back + service - 1, travel + service, travel, rank, service, retaining});
                }
            }
        }
        if (pending < 24) deposit();
        result.length = time;
        result.cargo = cargo; result.end = point;
        const int overflow = std::max(0, time - 24);
        result.cost = 100000.0 * overflow * overflow + 10000.0 * result.late +
            4.0 * result.travel + 0.2 * (time - releases[worker]) * (time - releases[worker]);
        if (return_dp && delivery_score == 0 && (overflow || result.late)) return improve_returns(route, worker, result);
        return result;
    }

    double return_score(const State& state, const std::vector<int>& route_delay) const {
        // Match aggregate deadlines to returns between useful jobs. These are
        // cheap estimates, not fixed producer or shed-visit constraints.
        Stock delivered{};
        Stock uninserted = production;
        for (const auto& route : state.route_scores)
            for (int item = 0; item < kag::N_ITEMS; ++item) uninserted[item] -= route.produced[item];
        std::vector<bool> used(state.routes.size());
        std::vector<int> last_rank(state.routes.size(), -1), added_delay(state.routes.size());
        std::vector<Stock> worker_delivered(state.routes.size());
        double cost = 0;
        for (int hour = 0; hour < 24; ++hour) {
            if (hour < 23 && (!hour || data.problem.shed_availability[hour] == data.problem.shed_availability[hour - 1])) continue;
            Stock missing{};
            for (int item = 0; item < kag::N_ITEMS; ++item) {
                int picked = 0;
                for (const auto& route : state.route_scores) if (route.pickup_hour[item] <= hour) picked += route.inputs[item];
                missing[item] = std::clamp<dc::Count>(data.problem.shed_availability[hour][item] + picked -
                    data.problem.start.shed[item] - data.bought(item, hour) - delivered[item], 0, std::numeric_limits<int>::max());
            }
            for (;;) {
                int best_worker = -1, best_offer = -1, best_delay = 0;
                double best_ratio = 1e100, best_cost = 0;
                for (int worker = 0; worker < int(state.routes.size()); ++worker) {
                    if (used[worker] && delivery_score == 2) continue;
                    const auto& route = state.route_scores[worker];
                    for (int index = 0; index < int(route.offers.size()); ++index) {
                        const auto& offer = route.offers[index];
                        if (offer.rank <= last_rank[worker]) continue;
                        int benefit = 0, types = 0;
                        for (int item = 0; item < kag::N_ITEMS; ++item) {
                            const int amount = std::max(0, offer.cargo[item] - worker_delivered[worker][item]);
                            benefit += std::min(missing[item], amount); types += amount > 0;
                        }
                        if (!benefit) continue;
                        const int service = offer.retaining ? types : 1;
                        if (offer.hour - offer.service + service + route_delay[worker] + added_delay[worker] > hour) continue;
                        const int delay = offer.delay - offer.service + service;
                        const int length = route.length + route_delay[worker] + added_delay[worker], end = length + delay;
                        const int before = std::max(0, length - 24), after = std::max(0, end - 24);
                        const double extra = 100000.0 * (after * after - before * before) + 4 * offer.travel +
                            0.2 * ((end - releases[worker]) * (end - releases[worker]) -
                            (length - releases[worker]) * (length - releases[worker]));
                        if (extra / benefit < best_ratio) {
                            best_worker = worker; best_offer = index; best_ratio = extra / benefit; best_cost = extra; best_delay = delay;
                        }
                    }
                }
                if (best_worker < 0) break;
                used[best_worker] = true; cost += best_cost;
                last_rank[best_worker] = state.route_scores[best_worker].offers[best_offer].rank;
                added_delay[best_worker] += best_delay;
                for (int item = 0; item < kag::N_ITEMS; ++item) {
                    const int amount = std::max(0, state.route_scores[best_worker].offers[best_offer].cargo[item] - worker_delivered[best_worker][item]);
                    worker_delivered[best_worker][item] += amount;
                    delivered[item] += amount; missing[item] = std::max(0, missing[item] - amount);
                }
            }
            for (int item = 0; item < kag::N_ITEMS; ++item) {
                const int shortfall = std::max(0, missing[item] - uninserted[item]);
                cost += 10000.0 * shortfall * shortfall;
            }
        }
        return cost;
    }

    double score(const State& state, int variant) const {
        double cost = state.route_cost;
        // Match seed supply to the latest possible planting slots implied by
        // each route's remaining travel and work. No plant is assigned a seed
        // in advance; a deficit proves these route orders need to change.
        for (const auto& [crop, available] : seed_supply) if (!alternate_purchase_scores || variant == 1) {
            std::array<int, 24> deadlines{};
            for (const auto& route : state.route_scores)
                for (const auto& [item, hour] : route.seed_deadlines)
                    if (item == crop) ++deadlines[hour];
            int required = 0;
            for (int hour = 0; hour < 24; ++hour) {
                required += deadlines[hour];
                const int deficit = std::max(0, required - available[hour]);
                cost += 10000.0 * deficit * deficit;
            }
        }
        // The completion model uses one pickup per route and item, before all
        // unmatched consumers. Match those quantities to staged purchases.
        for (const auto& [item, available] : input_supply) if (!alternate_purchase_scores || variant == 1) {
            std::array<int, 24> deadlines{};
            for (const auto& route : state.route_scores)
                for (const auto& need : route.input_deadlines)
                    if (need[0] == item) deadlines[need[1]] += need[2];
            int required = 0;
            for (int hour = 0; hour < 24; ++hour) {
                required += deadlines[hour];
                const int deficit = std::max(0, required - available[hour]);
                cost += 10000.0 * deficit * deficit;
            }
        }
        std::vector<int> route_delay(state.routes.size());
        if (split_jobs) {
            std::vector<int> start(jobs.size()), end(jobs.size()), owner(jobs.size(), -1), delay(jobs.size());
            for (int worker = 0; worker < int(state.routes.size()); ++worker)
                for (int rank = 0; rank < int(state.routes[worker].size()); ++rank) {
                    const int job = state.routes[worker][rank];
                    start[job] = state.route_scores[worker].spans[rank].first;
                    end[job] = state.route_scores[worker].spans[rank].second;
                    owner[job] = worker;
                }
            for (int pass = 0; pass <= int(jobs.size()); ++pass) {
                bool changed = false;
                auto update = [&](int job, int value) {
                    if (value > delay[job]) { delay[job] = value; changed = true; }
                };
                for (const auto& route : state.routes)
                    for (int rank = 1; rank < int(route.size()); ++rank) update(route[rank], delay[route[rank - 1]]);
                for (int job = 0; job < int(jobs.size()); ++job) {
                    const int previous = jobs[job].predecessor;
                    if (previous < 0 || owner[previous] < 0 || owner[job] < 0) continue;
                    // Workers act in index order; later workers may continue
                    // another worker's tile in the same hour.
                    const int required = end[previous] - (owner[previous] < owner[job]) + delay[previous];
                    update(job, required - start[job]);
                }
                if (!changed) break;
                if (pass == int(jobs.size())) return 1e12;
            }
            for (int worker = 0; worker < int(state.routes.size()); ++worker) {
                if (state.routes[worker].empty()) continue;
                const int shift = delay[state.routes[worker].back()], length = state.route_scores[worker].length;
                route_delay[worker] = shift;
                const int before = std::max(0, length - 24), after = std::max(0, length + shift - 24);
                cost += 100000.0 * (after * after - before * before) + 100 * shift * shift;
            }
        }
        Stock missing{};
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            const int deficit = std::max(0, state.inputs[item] - supply[item]);
            cost += (variant % 2 ? 20000.0 : 2000.0) * deficit * deficit;
            cost += 2 * state.inputs[item];
            int returned = 0;
            for (const auto& route : state.route_scores) returned += route.returned[item];
            missing[item] = std::clamp<dc::Count>(data.problem.shed_availability[23][item] + state.inputs[item] -
                data.problem.start.shed[item] - data.bought(item, 23) - returned, 0, std::numeric_limits<int>::max());
        }
        if (delivery_score >= 2) return cost + return_score(state, route_delay);
        // A return may replenish inputs consumed by other workers even when
        // the original shed stock alone exceeded the sales requirement.
        std::vector<bool> used(state.routes.size());
        for (;;) {
            int best = -1; double best_ratio = 1e100, best_cost = 0;
            for (int worker = 0; worker < int(state.routes.size()); ++worker) {
                if (used[worker] || state.routes[worker].empty()) continue;
                const auto& route = state.route_scores[worker];
                int benefit = 0;
                for (int item = 0; item < kag::N_ITEMS; ++item) benefit += std::min(missing[item], route.cargo[item]);
                if (!benefit) continue;
                const int length = route.length + route_delay[worker];
                const int travel = dc::tail(route.end), end = length + travel + 1;
                const int before = std::max(0, length - 24), after = std::max(0, end - 24);
                const double extra = 100000.0 * (after * after - before * before) + 4 * travel +
                    0.2 * ((end - releases[worker]) * (end - releases[worker]) -
                    (length - releases[worker]) * (length - releases[worker]));
                if (extra / benefit < best_ratio) { best = worker; best_ratio = extra / benefit; best_cost = extra; }
            }
            if (best < 0) break;
            used[best] = true; cost += best_cost;
            for (int item = 0; item < kag::N_ITEMS; ++item)
                missing[item] = std::max(0, missing[item] - state.route_scores[best].cargo[item]);
        }
        // Partial beams have not inserted all producers yet. Reward available
        // return capacity without treating their missing output as infeasible.
        return cost;
    }
    uint64_t hash(const State& state, bool completed = false) const {
        std::vector<int> order(state.routes.size());
        std::iota(order.begin(), order.end(), 0);
        auto profile = [&](int worker) { return std::tuple(starts[worker], releases[worker]); };
        std::sort(order.begin(), order.end(), [&](int a, int b) {
            if (!completed && profile(a) != profile(b)) return profile(a) < profile(b);
            return state.routes[a] < state.routes[b];
        });
        uint64_t value = 14695981039346656037ULL;
        for (int worker : order) {
            value = (value ^ 0xffff) * 1099511628211ULL;
            for (int job : state.routes[worker]) value = (value ^ uint64_t(job + 1)) * 1099511628211ULL;
        }
        return value;
    }
    bool insertion_order(const Route& route, int position, int job) const {
        const auto& tasks = data.tasks;
        const int first = jobs[job].tasks.front(), last = jobs[job].tasks.back();
        for (int offset = 0; offset < int(route.size()); ++offset) {
            const auto& other = jobs[route[offset]].tasks;
            if (tasks[other.front()].pattern != tasks[first].pattern) continue;
            if (offset < position && other.front() > last) return false;
            if (offset >= position && other.back() < first) return false;
        }
        return true;
    }
    bool valid_order(const Route& route) const {
        for (int rank = 0; rank < int(route.size()); ++rank) {
            auto rest = route; const int job = rest[rank]; rest.erase(rest.begin() + rank);
            if (!insertion_order(rest, rank, job)) return false;
        }
        return true;
    }
    void refresh(State& state, int worker) const {
        const auto updated = evaluate(state.routes[worker], worker);
        state.route_cost += updated.cost - state.route_scores[worker].cost;
        for (int item = 0; item < kag::N_ITEMS; ++item)
            state.inputs[item] += updated.inputs[item] - state.route_scores[worker].inputs[item];
        state.route_scores[worker] = updated;
    }
    std::array<double, 28> dispatch_features(const Route& prefix, int worker, int job) const {
        const auto route = evaluate(prefix, worker);
        const auto& next = jobs[job];
        const auto point = prefix.empty() ? starts[worker] : jobs[prefix.back()].point;
        const int dx = next.point[0] - point[0], dy = next.point[1] - point[1];
        const int distance = std::abs(dx) + std::abs(dy);
        const int time = prefix.empty() ? releases[worker] : route.length;
        Stock balance{}, need{}, produced{};
        int animal = 0, plant = 0, earliest = 0;
        for (int id : next.tasks) {
            const auto& task = data.tasks[id];
            if (task.input >= 0) { --balance[task.input]; need[task.input] = std::max(need[task.input], -balance[task.input]); }
            if (task.output >= 0) { balance[task.output] += task.quantity; produced[task.output] += task.quantity; }
            animal |= task.op == kag::OP_FEED || task.op == kag::OP_CARE;
            plant |= task.op == kag::OP_PLANT;
            earliest = std::max(earliest, data.early[id]);
        }
        int reused = 0, types = 0;
        for (int item = 0; item < kag::N_ITEMS; ++item) {
            reused += std::min(route.cargo[item], need[item]);
            types += need[item] > route.cargo[item];
        }
        double reversal = 0;
        if (prefix.size() >= 2) {
            const auto before = jobs[prefix[prefix.size() - 2]].point;
            reversal = std::max(0, -(point[0] - before[0]) * dx - (point[1] - before[1]) * dy) / 20.0;
        }
        const double duration = next.tasks.size() / 10.0;
        const double tail = dc::tail(next.point) / 10.0;
        const double slack = (24 - time - distance - int(next.tasks.size())) / 24.0;
        return {distance / 10.0, distance * distance / 100.0, double(dx == 0), double(dy == 0),
            tail, (dc::tail(next.point) - dc::tail(point)) / 10.0,
            dc::distance(starts[worker], next.point) / 10.0,
            double((next.point[0] < 5) == (starts[worker][0] < 5) && (next.point[1] < 5) == (starts[worker][1] < 5)),
            reversal, duration, duration * duration, time / 24.0, slack, std::min(0.0, slack),
            need[kag::WHEAT] / 5.0, need[kag::FERTILIZER] / 5.0,
            std::max(0, need[kag::WHEAT] - route.cargo[kag::WHEAT]) / 5.0,
            std::max(0, need[kag::FERTILIZER] - route.cargo[kag::FERTILIZER]) / 5.0,
            produced[kag::WHEAT] / 10.0, produced[kag::FERTILIZER] / 5.0,
            reused / 5.0, types / 5.0, next.due / 24.0, double(prefix.empty()),
            prefix.empty() ? distance / 10.0 : 0, prefix.empty() ? tail : 0,
            double(animal), double(plant) + std::max(0, earliest - time - distance) / 24.0};
    }
    std::vector<State> recombine(const std::vector<State>& pool, int variant,
                                std::chrono::steady_clock::time_point deadline, bool reassign) const {
        namespace sat = operations_research::sat;
        struct Column { int worker; Route route; double cost; };
        std::vector<Column> columns;
        std::map<std::pair<int, Route>, int> indices;
        const int workers = starts.size();
        auto add = [&](int worker, const Route& route, double cost) {
            dc::require(std::isfinite(cost) && cost >= 0, "invalid route column cost");
            if (indices.emplace(std::pair(worker, route), columns.size()).second)
                columns.push_back({worker, route, cost});
        };
        for (int worker = 0; worker < workers; ++worker) add(worker, {}, 0);
        for (const auto& state : pool) for (int worker = 0; worker < workers; ++worker)
            add(worker, state.routes[worker], state.route_scores[worker].cost);
        if (reassign) {
            std::set<Route> routes;
            for (const auto& state : pool) for (const auto& route : state.routes)
                if (!route.empty()) routes.insert(route);
            for (const auto& route : routes) {
                if (std::chrono::steady_clock::now() >= deadline) break;
                for (int worker = 0; worker < workers; ++worker)
                    if (!indices.contains({worker, route})) add(worker, route, evaluate(route, worker).cost);
            }
        }
        sat::CpModelBuilder model;
        std::vector<sat::BoolVar> selected;
        std::vector<std::vector<sat::BoolVar>> by_worker(workers), by_job(jobs.size());
        double total_cost = 1;
        for (const auto& column : columns) total_cost += column.cost;
        const double scale = std::min(100.0, 1e12 / total_cost);
        std::vector<int64_t> costs;
        for (const auto& column : columns) {
            const auto choice = model.NewBoolVar();
            selected.push_back(choice);
            by_worker[column.worker].push_back(choice);
            for (int job : column.route) by_job[job].push_back(choice);
            costs.push_back(std::llround(column.cost * scale));
        }
        for (const auto& choices : by_worker) model.AddExactlyOne(choices);
        for (const auto& choices : by_job) model.AddExactlyOne(choices);
        model.Minimize(sat::LinearExpr::WeightedSum(selected, costs));
        const auto& warm = pool.back();
        std::set<int> warm_columns;
        for (int worker = 0; worker < workers; ++worker)
            warm_columns.insert(indices.at({worker, warm.routes[worker]}));
        for (int column = 0; column < int(columns.size()); ++column)
            model.AddHint(selected[column], warm_columns.contains(column));
        std::vector<State> alternatives;
        for (int attempt = 0; attempt < 3; ++attempt) {
            const double left = std::chrono::duration<double>(deadline - std::chrono::steady_clock::now()).count();
            if (left <= 0) break;
            sat::SatParameters parameters;
            parameters.set_max_time_in_seconds(left);
            parameters.set_num_search_workers(1);
            parameters.set_random_seed(17);
            parameters.set_cp_model_probing_level(0);
            parameters.set_max_presolve_iterations(1);
            sat::Model solver;
            solver.Add(sat::NewSatParameters(parameters));
            const auto response = sat::SolveCpModel(model.Build(), &solver);
            dc::require(response.status() != sat::MODEL_INVALID, response.solution_info());
            if (response.status() != sat::OPTIMAL && response.status() != sat::FEASIBLE) break;
            State mixed;
            mixed.routes.resize(workers); mixed.route_scores.resize(workers);
            std::vector<sat::BoolVar> chosen;
            std::vector<int> covered(jobs.size());
            for (int column = 0; column < int(columns.size()); ++column) {
                if (!sat::SolutionBooleanValue(response, selected[column])) continue;
                chosen.push_back(selected[column]);
                const auto& value = columns[column];
                mixed.routes[value.worker] = value.route;
                for (int job : value.route) ++covered[job];
            }
            dc::require(int(chosen.size()) == workers &&
                std::ranges::all_of(covered, [](int count) { return count == 1; }), "invalid route recombination");
            for (int worker = 0; worker < workers; ++worker) refresh(mixed, worker);
            mixed.cost = score(mixed, variant);
            alternatives.push_back(std::move(mixed));
            model.AddLessOrEqual(sat::LinearExpr::Sum(chosen), workers - 1);
            model.ClearHints();
        }
        std::stable_sort(alternatives.begin(), alternatives.end(), [](const auto& a, const auto& b) { return a.cost < b.cost; });
        return alternatives;
    }
    State polish(State state, int variant, std::chrono::steady_clock::time_point deadline,
                 std::vector<State>* history = nullptr, bool chains = false) const {
        for (int round = 0; round < 30; ++round) {
            auto best = state;
            auto consider = [&](State candidate, int a, int b) {
                if (!valid_order(candidate.routes[a]) || (b != a && !valid_order(candidate.routes[b]))) return;
                refresh(candidate, a); if (b != a) refresh(candidate, b);
                candidate.cost = score(candidate, variant);
                if (candidate.cost < best.cost - 1e-6) {
                    if (history) {
                        history->push_back(best);
                        if (history->size() > 8) history->erase(history->begin());
                    }
                    best = std::move(candidate);
                }
            };
            for (int a = 0; a < int(state.routes.size()); ++a) {
                if (std::chrono::steady_clock::now() >= deadline) return best;
                for (int rank = 0; rank < int(state.routes[a].size()); ++rank) {
                    auto removed = state; const int job = removed.routes[a][rank];
                    removed.routes[a].erase(removed.routes[a].begin() + rank);
                    for (int b = 0; b < int(state.routes.size()); ++b)
                        for (int position = 0; position <= int(removed.routes[b].size()); ++position) {
                            auto candidate = removed;
                            candidate.routes[b].insert(candidate.routes[b].begin() + position, job);
                            consider(std::move(candidate), a, b);
                        }
                    for (int b = a + 1; b < int(state.routes.size()); ++b)
                        for (int position = 0; position < int(state.routes[b].size()); ++position) {
                            auto candidate = state;
                            std::swap(candidate.routes[a][rank], candidate.routes[b][position]);
                            consider(std::move(candidate), a, b);
                        }
                }
            }
            if (chains && best.cost >= state.cost - 1e-6) {
                // Single-job moves can separate a producer from the consumers
                // using its cargo. Try larger moves only at that local optimum.
                for (int a = 0; a < int(state.routes.size()); ++a) {
                    for (int first = 0; first <= int(state.routes[a].size()); ++first) {
                        if (std::chrono::steady_clock::now() >= deadline) return best;
                        for (int last = first + 2; last <= int(state.routes[a].size()); ++last) {
                            auto candidate = state;
                            std::reverse(candidate.routes[a].begin() + first, candidate.routes[a].begin() + last);
                            consider(std::move(candidate), a, a);
                        }
                        for (int b = a + 1; b < int(state.routes.size()); ++b)
                            for (int cut = 0; cut <= int(state.routes[b].size()); ++cut) {
                                auto candidate = state;
                                candidate.routes[a].erase(candidate.routes[a].begin() + first, candidate.routes[a].end());
                                candidate.routes[a].insert(candidate.routes[a].end(), state.routes[b].begin() + cut, state.routes[b].end());
                                candidate.routes[b].erase(candidate.routes[b].begin() + cut, candidate.routes[b].end());
                                candidate.routes[b].insert(candidate.routes[b].end(), state.routes[a].begin() + first, state.routes[a].end());
                                consider(std::move(candidate), a, b);
                            }
                        for (int length = 2; length <= 3 && first + length <= int(state.routes[a].size()); ++length) {
                            auto removed = state;
                            removed.routes[a].erase(removed.routes[a].begin() + first, removed.routes[a].begin() + first + length);
                            for (int b = 0; b < int(state.routes.size()); ++b) {
                                if (std::chrono::steady_clock::now() >= deadline) return best;
                                for (int position = 0; position <= int(removed.routes[b].size()); ++position) {
                                    auto candidate = removed;
                                    candidate.routes[b].insert(candidate.routes[b].begin() + position,
                                        state.routes[a].begin() + first, state.routes[a].begin() + first + length);
                                    consider(std::move(candidate), a, b);
                                }
                            }
                        }
                    }
                }
            }
            if (best.cost >= state.cost - 1e-6) return state;
            state = std::move(best);
        }
        return state;
    }
    double remaining_cost(const State& state) const {
        std::vector<bool> used(jobs.size());
        for (const auto& route : state.routes) for (int job : route) used[job] = true;
        std::vector<int> remaining;
        int work = 0, capacity = 0, late = 0;
        for (int job = 0; job < int(jobs.size()); ++job) if (!used[job]) {
            remaining.push_back(job); work += jobs[job].tasks.size();
        }
        std::vector<int> distance(remaining.size(), 100);
        for (int worker = 0; worker < int(starts.size()); ++worker)
            capacity += std::max(0, 24 - (state.routes[worker].empty() ? releases[worker] : state.route_scores[worker].length));
        for (int index = 0; index < int(remaining.size()); ++index) {
            const auto& job = jobs[remaining[index]];
            int earliest = 100;
            for (int worker = 0; worker < int(starts.size()); ++worker) {
                const auto& route = state.routes[worker];
                const auto point = route.empty() ? starts[worker] : jobs[route.back()].point;
                const int travel = dc::distance(point, job.point);
                distance[index] = std::min(distance[index], travel);
                earliest = std::min(earliest, (route.empty() ? releases[worker] : state.route_scores[worker].length) +
                    travel + int(job.tasks.size()));
            }
            late += std::max(0, earliest - 24) * std::max(0, earliest - 24);
        }
        // Rooted Manhattan spanning forest: optimistic remaining travel from
        // all worker endpoints. This ranks states, never proves infeasibility.
        int travel = 0;
        for (int step = 0; step < int(remaining.size()); ++step) {
            const int selected = int(std::min_element(distance.begin(), distance.end()) - distance.begin());
            travel += distance[selected]; distance[selected] = 1000;
            for (int index = 0; index < int(remaining.size()); ++index)
                if (distance[index] < 1000)
                    distance[index] = std::min(distance[index], dc::distance(jobs[remaining[selected]].point, jobs[remaining[index]].point));
        }
        const int shortage = std::max(0, work + travel - capacity);
        return 4.0 * travel + 10000.0 * late + 100000.0 * shortage * shortage;
    }

    std::vector<State> dispatch_beam(int width, int variant, std::chrono::steady_clock::time_point deadline, bool fitted, bool lookahead) {
        // Reserve a whole job for the next available worker, branching over
        // useful jobs rather than individual moves. This permits several
        // workers' routes to develop together instead of completing one first.
        struct Node { State state; double prior = 0, future = 0; };
        const auto started = std::chrono::steady_clock::now();
        const auto beam_deadline = started + (deadline - started) * 3 / 4;
        State initial; initial.routes.resize(starts.size()); initial.route_scores.resize(starts.size());
        std::vector<Node> beam{{std::move(initial), 0}};
        depth = 0;
        const double prior_weight = std::array{0.0, 8.0, 32.0, 128.0}[variant];
        for (int layer = 0; layer < int(jobs.size()) && std::chrono::steady_clock::now() < beam_deadline; ++layer) {
            std::vector<Node> candidates;
            for (const auto& node : beam) {
                if (std::chrono::steady_clock::now() >= beam_deadline) break;
                const auto& state = node.state;
                std::vector<bool> used(jobs.size());
                for (const auto& route : state.routes) for (int job : route) used[job] = true;
                std::vector<int> workers(starts.size()); std::iota(workers.begin(), workers.end(), 0);
                auto time = [&](int worker) {
                    return state.routes[worker].empty() ? releases[worker] : state.route_scores[worker].length;
                };
                std::stable_sort(workers.begin(), workers.end(), [&](int a, int b) { return time(a) < time(b); });
                for (int worker : workers) {
                    struct Choice { int job; RouteScore route; double priority; };
                    std::vector<Choice> choices;
                    const auto& route = state.routes[worker];
                    for (int job = 0; job < int(jobs.size()); ++job) {
                        if (used[job] || (jobs[job].predecessor >= 0 && !used[jobs[job].predecessor]) ||
                            !insertion_order(route, route.size(), job)) continue;
                        auto updated_route = route; updated_route.push_back(job);
                        auto updated = evaluate(updated_route, worker); ++generated;
                        if (updated.length > 24 || updated.late) continue;
                        const auto features = dispatch_features(route, worker, job);
                        const double priority = fitted ? std::inner_product(features.begin(), features.end(),
                            dispatch_model::weights.begin(), 0.0) :
                            -10 * features[0] + 4 * features[9] + 2 * features[20] - 2 * features[21];
                        choices.push_back({job, std::move(updated), priority});
                    }
                    if (choices.empty()) continue;
                    std::stable_sort(choices.begin(), choices.end(), [](const auto& a, const auto& b) { return a.priority > b.priority; });
                    double normalizer = 0;
                    for (const auto& choice : choices) normalizer += std::exp(choice.priority - choices.front().priority);
                    const double log_normalizer = choices.front().priority + std::log(normalizer);
                    for (int index = 0; index < std::min<int>(8, choices.size()); ++index) {
                        const auto& choice = choices[index];
                        auto candidate = state;
                        candidate.routes[worker].push_back(choice.job);
                        candidate.route_cost += choice.route.cost - candidate.route_scores[worker].cost;
                        for (int item = 0; item < kag::N_ITEMS; ++item)
                            candidate.inputs[item] += choice.route.inputs[item] - candidate.route_scores[worker].inputs[item];
                        candidate.route_scores[worker] = choice.route;
                        candidate.cost = score(candidate, variant);
                        const double future = lookahead ? remaining_cost(candidate) : 0;
                        candidates.push_back({std::move(candidate), node.prior + log_normalizer - choice.priority, future});
                    }
                    break;
                }
            }
            if (candidates.empty()) break;
            auto value = [&](const Node& node) { return node.state.cost + prior_weight * node.prior + node.future; };
            std::stable_sort(candidates.begin(), candidates.end(), [&](const auto& a, const auto& b) { return value(a) < value(b); });
            std::unordered_set<uint64_t> seen;
            beam.clear();
            for (auto& candidate : candidates) {
                if (!seen.insert(hash(candidate.state)).second) continue;
                beam.push_back(std::move(candidate));
                if (int(beam.size()) == width) break;
            }
            ++depth;
        }
        std::vector<State> result;
        if (depth == int(jobs.size())) for (auto& node : beam) result.push_back(std::move(node.state));
        else for (int index = 0; index < std::min<int>(8, beam.size()) && std::chrono::steady_clock::now() < deadline; ++index) {
            auto repaired = regret(variant, deadline, std::move(beam[index].state));
            if (!repaired.empty()) result.push_back(std::move(repaired.front()));
        }
        std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.cost < b.cost; });
        return result;
    }

    std::vector<State> dispatch(int variant, std::chrono::steady_clock::time_point deadline, bool fitted, bool columns) {
        // Pack a worker's whole route before moving to another worker. Multiple
        // cheap rollouts vary worker order and next-job choice; only our own
        // partial routes are passed to residual insertion and exact completion.
        const auto started = std::chrono::steady_clock::now();
        const auto rollout_deadline = started + (deadline - started) * (columns ? 2 : 3) / 4;
        std::mt19937 random(173 + variant);
        std::uniform_real_distribution<double> uniform(1e-9, 1 - 1e-9);
        std::vector<State> complete, partial, route_pool;
        std::unordered_set<uint64_t> seen;
        auto keep = [&](std::vector<State>& pool, State state) {
            pool.push_back(std::move(state));
            std::stable_sort(pool.begin(), pool.end(), [](const auto& a, const auto& b) { return a.cost < b.cost; });
            if (pool.size() > 8) pool.resize(8);
        };
        depth = 0;
        dispatch_rollouts = dispatch_mixed = 0;
        for (int trial = 0; trial < 256 && std::chrono::steady_clock::now() < rollout_deadline; ++trial) {
            ++dispatch_rollouts;
            State state; state.routes.resize(starts.size()); state.route_scores.resize(starts.size());
            std::vector<bool> used(jobs.size());
            std::vector<int> workers(starts.size()); std::iota(workers.begin(), workers.end(), 0);
            if (trial % 4 == 1) std::reverse(workers.begin(), workers.end());
            if (trial % 4 >= 2) std::shuffle(workers.begin(), workers.end(), random);
            const double temperature = trial < 2 ? 0 : std::array{0.15, 0.3, 0.5, 0.8}[trial % 4];
            int inserted = 0;
            for (int worker : workers) {
                for (;;) {
                    if (std::chrono::steady_clock::now() >= rollout_deadline) break;
                    const auto& route = state.routes[worker];
                    int selected = -1;
                    double best = -1e100;
                    for (int job = 0; job < int(jobs.size()); ++job) {
                        if (used[job] || (jobs[job].predecessor >= 0 && !used[jobs[job].predecessor]) ||
                            !insertion_order(route, route.size(), job)) continue;
                        auto candidate = route; candidate.push_back(job);
                        const auto updated = evaluate(candidate, worker); ++generated;
                        if (updated.length > 24 || updated.late) continue;
                        const auto features = dispatch_features(route, worker, job);
                        double priority = fitted ? std::inner_product(features.begin(), features.end(),
                            dispatch_model::weights.begin(), 0.0) :
                            -10 * features[0] + 4 * features[9] + 2 * features[20] - 2 * features[21];
                        for (int item = 0; item < kag::N_ITEMS; ++item) {
                            const int before = std::max(0, state.inputs[item] - supply[item]);
                            const int after = std::max(0, state.inputs[item] + updated.inputs[item] -
                                state.route_scores[worker].inputs[item] - supply[item]);
                            priority -= 0.5 * (after - before);
                        }
                        priority -= temperature * std::log(-std::log(uniform(random)));
                        if (priority > best) { best = priority; selected = job; }
                    }
                    if (selected < 0) break;
                    state.routes[worker].push_back(selected); refresh(state, worker);
                    used[selected] = true; ++inserted;
                }
            }
            depth = std::max(depth, inserted);
            if (!seen.insert(hash(state, true)).second) continue;
            state.cost = score(state, variant);
            if (columns) route_pool.push_back(state);
            if (inserted == int(jobs.size())) keep(complete, std::move(state));
            else {
                int residual = 0;
                for (int job = 0; job < int(jobs.size()); ++job)
                    if (!used[job]) residual += int(jobs[job].tasks.size()) + 1;
                state.cost += 1e7 * residual;
                keep(partial, std::move(state));
            }
        }
        if (columns && !route_pool.empty() && std::chrono::steady_clock::now() < deadline) {
            const auto master_deadline = std::chrono::steady_clock::now() +
                (deadline - std::chrono::steady_clock::now()) * 3 / 4;
            auto mixed = recombine(route_pool, variant, master_deadline, false);
            dispatch_mixed = mixed.size();
            for (auto& state : mixed) keep(complete, std::move(state));
        }
        for (auto& state : partial) {
            if (std::chrono::steady_clock::now() >= deadline) break;
            state.cost = score(state, variant);
            auto repaired = regret(variant, deadline, std::move(state));
            if (!repaired.empty()) keep(complete, std::move(repaired.front()));
        }
        return complete;
    }

    std::vector<State> merge_routes(int variant, std::chrono::steady_clock::time_point deadline, int rollout = 0) {
        // Begin with individual jobs and merge whole chains, then match the
        // remaining chains to real workers. Merges account for input reuse;
        // exact timing and stock completion still decide feasibility.
        struct Chain { Route route; RouteScore score; bool active = true; };
        struct Merge { int left, right, worker; double cost; Stock inputs; };
        std::vector<Chain> chains;
        std::vector<Merge> merges;
        std::mt19937 random(173 + variant * 1009 + rollout * 9176);
        std::vector<int> profiles;
        for (int worker = 0; worker < int(starts.size()); ++worker)
            if (std::ranges::none_of(profiles, [&](int other) {
                return starts[worker] == starts[other] && releases[worker] == releases[other];
            })) profiles.push_back(worker);
        auto best_route = [&](const Route& route) {
            int worker = profiles.front();
            auto best = evaluate(route, worker);
            for (int other : profiles) if (other != worker) {
                auto value = evaluate(route, other);
                if (value.cost < best.cost) { worker = other; best = std::move(value); }
            }
            return std::pair(worker, std::move(best));
        };
        Stock inputs{};
        for (int job = 0; job < int(jobs.size()); ++job) {
            auto [worker, value] = best_route({job});
            for (int item = 0; item < kag::N_ITEMS; ++item) inputs[item] += value.inputs[item];
            chains.push_back({{job}, std::move(value)});
        }
        auto joined = [&](int left, int right) {
            auto route = chains[left].route;
            for (int job : chains[right].route) {
                if (!insertion_order(route, route.size(), job)) return Route{};
                route.push_back(job);
            }
            return route;
        };
        auto add_merge = [&](int left, int right) {
            if (std::chrono::steady_clock::now() >= deadline) return false;
            auto route = joined(left, right);
            if (route.empty()) return true;
            auto [worker, value] = best_route(route); ++generated;
            const int link = dc::distance(jobs[chains[left].route.back()].point, jobs[chains[right].route.front()].point);
            const double cost = value.cost - chains[left].score.cost - chains[right].score.cost + (variant >= 2 ? 4 * link : 0);
            merges.push_back({left, right, worker, cost, value.inputs});
            return true;
        };
        int active = chains.size();
        if (active > int(starts.size()))
            for (int left = 0; left < active; ++left) for (int right = 0; right < active; ++right)
                if (left != right && !add_merge(left, right)) return {};
        depth = 0;
        while (active > int(starts.size())) {
            if (std::chrono::steady_clock::now() >= deadline) return {};
            std::array<std::pair<double, int>, 4> best;
            best.fill({std::numeric_limits<double>::infinity(), -1});
            for (int index = 0; index < int(merges.size()); ++index) {
                const auto& merge = merges[index];
                if (!chains[merge.left].active || !chains[merge.right].active) continue;
                double cost = merge.cost;
                for (int item = 0; item < kag::N_ITEMS; ++item) {
                    const int next = inputs[item] + merge.inputs[item] - chains[merge.left].score.inputs[item] - chains[merge.right].score.inputs[item];
                    const int before = std::max(0, inputs[item] - supply[item]), after = std::max(0, next - supply[item]);
                    cost += (variant % 2 ? 20000.0 : 2000.0) * (after * after - before * before);
                }
                const std::pair value{cost, index};
                if (value < best.back()) {
                    best.back() = value;
                    std::sort(best.begin(), best.end());
                }
            }
            int choice = 0;
            if (rollout) {
                const auto draw = random() % 100;
                choice = (draw >= 65) + (draw >= 85) + (draw >= 95);
                while (choice && best[choice].second < 0) --choice;
            }
            const int selected = best[choice].second;
            if (selected < 0) return {};
            const auto merge = merges[selected];
            auto route = joined(merge.left, merge.right);
            auto value = evaluate(route, merge.worker);
            for (int item = 0; item < kag::N_ITEMS; ++item)
                inputs[item] += value.inputs[item] - chains[merge.left].score.inputs[item] - chains[merge.right].score.inputs[item];
            chains[merge.left].active = chains[merge.right].active = false;
            const int added = chains.size();
            chains.push_back({std::move(route), std::move(value)});
            --active; ++depth;
            if (active > int(starts.size())) for (int other = 0; other < added; ++other)
                if (chains[other].active && (!add_merge(added, other) || !add_merge(other, added))) return {};
        }
        std::vector<int> ids;
        std::vector<double> costs;
        for (int id = 0; id < int(chains.size()); ++id) if (chains[id].active) {
            ids.push_back(id);
            for (int worker = 0; worker < int(starts.size()); ++worker)
                costs.push_back(evaluate(chains[id].route, worker).cost);
        }
        std::vector<int64_t> rows(ids.size()), columns(ids.size());
        dc::require(solve_rectangular_linear_sum_assignment(ids.size(), starts.size(), costs.data(), false,
            rows.data(), columns.data()) == 0, "chain-to-worker assignment failed");
        State state; state.routes.resize(starts.size()); state.route_scores.resize(starts.size());
        for (int index = 0; index < int(ids.size()); ++index) {
            const int worker = columns[index];
            state.routes[worker] = std::move(chains[ids[rows[index]]].route);
            refresh(state, worker);
        }
        state.cost = score(state, variant);
        depth = jobs.size();
        return {std::move(state)};
    }

    std::vector<State> merge_portfolio(int variant, std::chrono::steady_clock::time_point deadline, int rollouts) {
        std::vector<State> result;
        std::unordered_set<uint64_t> seen;
        for (int rollout = 0; rollout < rollouts && std::chrono::steady_clock::now() < deadline; ++rollout) {
            auto candidates = merge_routes(variant, deadline, rollout);
            for (auto& candidate : candidates)
                if (seen.insert(hash(candidate, true)).second) result.push_back(std::move(candidate));
        }
        std::stable_sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.cost < b.cost; });
        if (result.size() > 8) result.resize(8);
        return result;
    }

    std::vector<State> regret(int variant, std::chrono::steady_clock::time_point deadline, State state = {}) {
        if (state.routes.empty()) { state.routes.resize(starts.size()); state.route_scores.resize(starts.size()); }
        std::vector<bool> inserted(jobs.size());
        depth = 0;
        for (const auto& route : state.routes) for (int job : route) { inserted[job] = true; ++depth; }
        while (depth < int(jobs.size())) {
            int selected = -1, selected_worker = -1, selected_position = -1;
            double selected_cost = 0;
            std::tuple<double, int, int, int> best_priority;
            for (int job = 0; job < int(jobs.size()); ++job) {
                if (std::chrono::steady_clock::now() >= deadline) return {};
                if (inserted[job] || (jobs[job].predecessor >= 0 && !inserted[jobs[job].predecessor])) continue;
                auto candidate = state;
                std::vector<double> worker_cost(starts.size(), 1e100);
                double best = 1e100;
                int best_worker = -1, best_position = -1;
                for (int worker = 0; worker < int(starts.size()); ++worker) {
                    const auto& route = state.routes[worker];
                    auto& changed = candidate.routes[worker];
                    for (int position = 0; position <= int(route.size()); ++position) {
                        if (!insertion_order(route, position, job)) continue;
                        changed.insert(changed.begin() + position, job);
                        const auto updated = evaluate(changed, worker);
                        candidate.route_cost = state.route_cost + updated.cost - state.route_scores[worker].cost;
                        for (int item = 0; item < kag::N_ITEMS; ++item)
                            candidate.inputs[item] = state.inputs[item] + updated.inputs[item] - state.route_scores[worker].inputs[item];
                        candidate.route_scores[worker] = updated;
                        const double cost = score(candidate, variant); ++generated;
                        worker_cost[worker] = std::min(worker_cost[worker], cost);
                        if (cost < best) { best = cost; best_worker = worker; best_position = position; }
                        changed.erase(changed.begin() + position);
                    }
                    candidate.route_scores[worker] = state.route_scores[worker];
                }
                std::sort(worker_cost.begin(), worker_cost.end());
                double regret = 0;
                // Keep jobs with few good worker choices from being postponed.
                // The two order families use regret-2 and regret-3 respectively.
                for (int alternative = 1; alternative < std::min<int>(variant < 2 ? 2 : 3, worker_cost.size()); ++alternative)
                    regret += worker_cost[alternative] - worker_cost.front();
                const auto priority = std::tuple(regret, -jobs[job].due, dc::tail(jobs[job].point), -job);
                if (selected < 0 || priority > best_priority) {
                    selected = job; selected_worker = best_worker; selected_position = best_position;
                    selected_cost = best; best_priority = priority;
                }
            }
            dc::require(selected >= 0 && selected_worker >= 0, "regret insertion lost an eligible job");
            auto& route = state.routes[selected_worker];
            route.insert(route.begin() + selected_position, selected);
            refresh(state, selected_worker); state.cost = selected_cost;
            inserted[selected] = true; ++depth;
        }
        return {std::move(state)};
    }

    std::vector<State> rebuild(const State& initial, int variant, std::chrono::steady_clock::time_point deadline, bool route_first, bool adaptive) {
        // Remove several related jobs and let regret insertion reconsider their
        // workers and order together. Preserve distinct complete alternatives;
        // the lowest approximate score need not have feasible timed inventory.
        std::mt19937 random(31 + variant);
        auto incumbent = initial, best = initial;
        std::vector<State> alternatives{initial};
        std::unordered_set<uint64_t> seen{hash(initial, true)};
        for (int iteration = 0; iteration < (adaptive ? 128 : 32) && std::chrono::steady_clock::now() < deadline; ++iteration) {
            auto candidate = iteration % 4 ? incumbent : adaptive ? best : initial;
            std::vector<int> order(jobs.size()); std::iota(order.begin(), order.end(), 0);
            const int anchor = random() % jobs.size();
            std::shuffle(order.begin(), order.end(), random);
            if (iteration % 3) std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
                return dc::distance(jobs[a].point, jobs[anchor].point) < dc::distance(jobs[b].point, jobs[anchor].point);
            });
            int count = std::min<int>(jobs.size(), 3 + iteration % 10);
            std::vector<bool> removed(jobs.size());
            for (int index = 0; index < count; ++index) removed[order[index]] = true;
            if (adaptive && iteration % 3 == 2) {
                // Empty a tight route and nearby jobs from other routes, so
                // reconstruction can move a whole sequence across workers.
                int worst = -1;
                for (int worker = 0; worker < int(starts.size()); ++worker)
                    if (!candidate.routes[worker].empty() && (worst < 0 ||
                        candidate.route_scores[worker].cost > candidate.route_scores[worst].cost)) worst = worker;
                const auto& route = candidate.routes[worst];
                const auto point = jobs[route[random() % route.size()]].point;
                std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
                    return dc::distance(jobs[a].point, point) < dc::distance(jobs[b].point, point);
                });
                std::fill(removed.begin(), removed.end(), false);
                for (int job : route) removed[job] = true;
                for (int index = 0; index < std::min<int>(6, order.size()); ++index) removed[order[index]] = true;
            }
            for (int worker = 0; worker < int(starts.size()); ++worker) {
                std::erase_if(candidate.routes[worker], [&](int job) { return removed[job]; });
                refresh(candidate, worker);
            }
            auto complete = regret(variant, deadline, std::move(candidate));
            if (complete.empty()) break;
            const double remaining = std::chrono::duration<double>(deadline - std::chrono::steady_clock::now()).count();
            if (remaining <= 0) break;
            auto polished = polish(std::move(complete.front()), variant, std::chrono::steady_clock::now() +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(std::min(0.08, remaining))));
            if (polished.cost < best.cost) best = polished;
            bool accept = polished.cost < incumbent.cost;
            if (adaptive && !accept) {
                const double temperature = std::max(1000.0, initial.cost * 0.1) * std::pow(0.95, iteration);
                const double draw = (double(random()) + 1) / (double(std::mt19937::max()) + 2);
                accept = polished.cost - incumbent.cost < -temperature * std::log(draw);
            }
            if (accept) incumbent = polished;
            if (!seen.insert(hash(polished, true)).second) continue;
            alternatives.push_back(std::move(polished));
            std::stable_sort(alternatives.begin(), alternatives.end(), [&](const auto& a, const auto& b) {
                return std::pair(route_first ? a.route_cost : a.cost, a.cost) <
                       std::pair(route_first ? b.route_cost : b.cost, b.cost);
            });
            if (alternatives.size() > 8) alternatives.pop_back();
        }
        return alternatives;
    }

    std::vector<State> run(int width, int variant, std::chrono::steady_clock::time_point deadline) {
        std::vector<int> order(jobs.size()); std::iota(order.begin(), order.end(), 0);
        auto priority = [&](int job) {
            const auto& value = jobs[job];
            int resources = 0;
            for (int task : value.tasks) if (data.tasks[task].output == kag::WHEAT || data.tasks[task].output == kag::FERTILIZER)
                resources += data.tasks[task].quantity;
            if (variant < 2) return std::tuple(value.due, -dc::tail(value.point), -int(value.tasks.size()), job);
            return std::tuple(value.due, -resources, -dc::tail(value.point), job);
        };
        std::sort(order.begin(), order.end(), [&](int a, int b) { return priority(a) < priority(b); });
        if (split_jobs) {
            std::vector<int> sorted;
            std::vector<bool> inserted(jobs.size());
            while (sorted.size() < order.size())
                for (int job : order) {
                    if (inserted[job] || (jobs[job].predecessor >= 0 && !inserted[jobs[job].predecessor])) continue;
                    sorted.push_back(job); inserted[job] = true;
                    break;
                }
            order = std::move(sorted);
        }
        State initial; initial.routes.resize(starts.size()); initial.route_scores.resize(starts.size());
        std::vector<State> beam{std::move(initial)};
        depth = 0;
        for (int job : order) {
            if (std::chrono::steady_clock::now() >= deadline) return {};
            struct Insertion { double cost; int parent, worker, position; };
            std::vector<Insertion> candidates;
            for (int parent = 0; parent < int(beam.size()); ++parent) {
                const auto& state = beam[parent];
                auto candidate = state;
                std::set<std::tuple<dc::Point, int>> empty;
                for (int worker = 0; worker < int(starts.size()); ++worker) {
                    const auto& route = state.routes[worker];
                    if (route.empty() && !empty.emplace(starts[worker], releases[worker]).second) continue;
                    auto& changed = candidate.routes[worker];
                    for (int position = 0; position <= int(route.size()); ++position) {
                        if (!insertion_order(route, position, job)) continue;
                        changed.insert(changed.begin() + position, job);
                        const auto updated = evaluate(changed, worker);
                        candidate.route_cost = state.route_cost + (updated.cost - state.route_scores[worker].cost);
                        for (int item = 0; item < kag::N_ITEMS; ++item)
                            candidate.inputs[item] = state.inputs[item] + updated.inputs[item] - state.route_scores[worker].inputs[item];
                        candidate.route_scores[worker] = updated;
                        candidate.cost = score(candidate, variant);
                        candidates.push_back({candidate.cost, parent, worker, position}); ++generated;
                        changed.erase(changed.begin() + position);
                    }
                    candidate.route_scores[worker] = state.route_scores[worker];
                }
            }
            auto less = [](const auto& a, const auto& b) { return a.cost < b.cost; };
            const int retained = std::min<int>(candidates.size(), width * 4);
            if (retained < int(candidates.size())) std::nth_element(candidates.begin(), candidates.begin() + retained, candidates.end(), less);
            std::sort(candidates.begin(), candidates.begin() + retained, less);
            std::unordered_set<uint64_t> seen;
            auto parents = std::move(beam);
            beam.clear(); beam.reserve(width);
            // Copy complete route sets only for the shortlist. The discarded
            // insertions need just a cost and the change to their parent.
            for (int index = 0; index < retained && int(beam.size()) < width; ++index) {
                const auto& insertion = candidates[index];
                auto candidate = parents[insertion.parent];
                auto& route = candidate.routes[insertion.worker];
                route.insert(route.begin() + insertion.position, job);
                refresh(candidate, insertion.worker); candidate.cost = insertion.cost;
                if (seen.insert(hash(candidate)).second) beam.push_back(std::move(candidate));
            }
            if (beam.empty()) return {};
            ++depth;
        }
        return beam;
    }
    dn::InternalHint hint(const State& state, bool timed) const {
        dn::InternalHint result;
        result.type_workers.emplace(1);
        for (int worker = 0; worker < int(starts.size()); ++worker) {
            result.type_workers->front().push_back(worker);
            int rank = 0;
            for (int index = 0; index < int(state.routes[worker].size()); ++index) {
                const int job = state.routes[worker][index];
                if (timed) rank = state.route_scores[worker].spans[index].first;
                for (int task : jobs[job].tasks) {
                    if (timed) rank = std::max(rank, data.early[task]);
                    result.assignments.push_back({task, worker, rank++, 0, {}});
                }
            }
        }
        return result;
    }
    std::string describe(const State& state) const {
        std::string result;
        for (int worker = 0; worker < int(state.routes.size()); ++worker) {
            if (state.routes[worker].empty()) continue;
            result += " w" + std::to_string(worker) + "[";
            for (int job : state.routes[worker]) {
                result += "{";
                for (int task : jobs[job].tasks) result += std::to_string(task) + ",";
                result += "}";
            }
            const auto& score = state.route_scores[worker];
            result += "]end=" + std::to_string(score.length) + " late=" + std::to_string(score.late);
        }
        return result;
    }
};
} // namespace day_scheduler::beam_detail

namespace day_scheduler {
Result job_route_witness(const day_solver::DayProblem& problem, const day_native::InternalHint& hint, int max_job_tasks, bool return_dp) {
    beam_detail::Search search(problem, max_job_tasks, 0, false, false, false, false, return_dp);
    std::vector<int> job_of(search.data.tasks.size()), owner(search.jobs.size(), -1);
    for (int job = 0; job < int(search.jobs.size()); ++job)
        for (int task : search.jobs[job].tasks) job_of[task] = job;
    std::vector<std::vector<std::pair<int, int>>> ordered(problem.worker_count);
    std::vector<bool> seen_task(job_of.size());
    for (const auto& task : hint.assignments) {
        beam_detail::dc::require(task.worker >= 0 && task.worker < problem.worker_count && task.task >= 0 &&
            task.task < int(job_of.size()) && !seen_task[task.task] && task.hour, "invalid diagnostic task order");
        seen_task[task.task] = true;
        ordered[task.worker].push_back({*task.hour, task.task});
    }
    beam_detail::dc::require(std::ranges::all_of(seen_task, [](bool seen) { return seen; }), "incomplete diagnostic task order");
    beam_detail::State state;
    state.routes.resize(problem.worker_count); state.route_scores.resize(problem.worker_count);
    int split = 0, interrupted = 0, late_selected = 0;
    for (const auto& task : hint.assignments)
        if (search.data.fixed_deadline.contains(task.task) && search.data.fixed_deadline.at(task.task) < 23 &&
            *task.hour + 1 + beam_detail::dc::tail(search.data.tasks[task.task].point) > search.data.fixed_deadline.at(task.task))
            ++late_selected;
    for (int worker = 0; worker < problem.worker_count; ++worker) {
        std::sort(ordered[worker].begin(), ordered[worker].end());
        std::unordered_set<int> seen_job;
        for (const auto& [hour, task] : ordered[worker]) {
            const int job = job_of[task];
            if (owner[job] >= 0 && owner[job] != worker) ++split;
            owner[job] = worker;
            auto& route = state.routes[worker];
            if (!route.empty() && route.back() == job) continue;
            if (!seen_job.insert(job).second) ++interrupted;
            route.push_back(job);
        }
    }
    Result result;
    result.stages.push_back({"diagnostic/representation", "jobs=" + std::to_string(search.jobs.size()) +
        " split_tasks=" + std::to_string(split) + " interrupted_jobs=" + std::to_string(interrupted), 0});
    result.stages.push_back({"diagnostic/selected_delivery_producers", "late_for_greedy_assignment=" + std::to_string(late_selected), 0});
    if (split || interrupted) return result;
    int late = 0, overtime = 0, travel = 0;
    for (int worker = 0; worker < problem.worker_count; ++worker) {
        search.refresh(state, worker);
        const auto& score = state.route_scores[worker];
        late += score.late; overtime += std::max(0, score.length - 24); travel += score.travel;
    }
    for (int variant = 0; variant < 4; ++variant)
        result.stages.push_back({"diagnostic/score" + std::to_string(variant), "cost=" + std::to_string(search.score(state, variant)) +
            " overtime=" + std::to_string(overtime) + " late=" + std::to_string(late) + " travel=" + std::to_string(travel) +
            " wheat=" + std::to_string(state.inputs[kag::WHEAT]) + " fertilizer=" + std::to_string(state.inputs[kag::FERTILIZER]), 0});
    result.stages.push_back({"diagnostic/routes", search.describe(state), 0});
    result.stages.push_back({"diagnostic/base_route_cost", std::to_string(state.route_cost), 0});
    search.data.fixed_deadline.clear();
    for (int worker = 0; worker < problem.worker_count; ++worker) search.refresh(state, worker);
    result.stages.push_back({"diagnostic/without_selected_returns", "cost=" + std::to_string(search.score(state, 0)) +
        " base_route_cost=" + std::to_string(state.route_cost), 0});
    search.delivery_score = 2;
    for (int worker = 0; worker < problem.worker_count; ++worker) search.refresh(state, worker);
    result.stages.push_back({"diagnostic/prefix_returns", "cost=" + std::to_string(search.score(state, 0)) +
        " base_route_cost=" + std::to_string(state.route_cost), 0});
    search.delivery_score = 3;
    result.stages.push_back({"diagnostic/multiple_prefix_returns", "cost=" + std::to_string(search.score(state, 0)), 0});
    return result;
}

std::string job_dispatch_examples(const day_solver::DayProblem& problem, const day_native::InternalHint& hint) {
    namespace bd = beam_detail;
    namespace json = boost::json;
    bd::Search search(problem, 0, 1);
    std::vector<int> job_of(search.data.tasks.size()), owner(search.jobs.size(), -1);
    for (int job = 0; job < int(search.jobs.size()); ++job)
        for (int task : search.jobs[job].tasks) job_of[task] = job;
    std::vector<std::vector<std::pair<int, int>>> ordered(problem.worker_count);
    std::vector<bool> seen_task(job_of.size());
    for (const auto& task : hint.assignments) {
        bd::dc::require(task.worker >= 0 && task.worker < problem.worker_count && task.task >= 0 &&
            task.task < int(job_of.size()) && !seen_task[task.task] && task.hour, "invalid dispatch training witness");
        seen_task[task.task] = true;
        ordered[task.worker].push_back({*task.hour, task.task});
    }
    bd::dc::require(std::ranges::all_of(seen_task, [](bool seen) { return seen; }), "incomplete dispatch training witness");
    json::array examples;
    for (int worker = 0; worker < problem.worker_count; ++worker) {
        std::sort(ordered[worker].begin(), ordered[worker].end());
        bd::Route route;
        std::vector<bool> used(search.jobs.size());
        for (const auto& [hour, task] : ordered[worker]) {
            const int job = job_of[task];
            bd::dc::require(owner[job] < 0 || owner[job] == worker, "split tile in dispatch training witness");
            owner[job] = worker;
            if (!route.empty() && route.back() == job) continue;
            bd::dc::require(!used[job], "revisited job in dispatch training witness");
            json::array choices;
            int target = -1;
            for (int candidate = 0; candidate < int(search.jobs.size()); ++candidate) {
                if (used[candidate] || !search.insertion_order(route, route.size(), candidate)) continue;
                if (candidate == job) target = choices.size();
                json::array features;
                for (double value : search.dispatch_features(route, worker, candidate)) features.push_back(value);
                choices.push_back(json::object{{"job", candidate}, {"features", std::move(features)}});
            }
            bd::dc::require(target >= 0, "observed dispatch choice excluded");
            examples.push_back(json::object{{"worker", worker}, {"hour", hour}, {"target", target},
                {"prefix_size", route.size()}, {"choices", std::move(choices)}});
            route.push_back(job); used[job] = true;
        }
    }
    return json::serialize(json::object{{"jobs", search.jobs.size()}, {"examples", std::move(examples)}});
}

Result job_beam(const day_solver::DayProblem& problem, const JobBeamOptions& options, JobBeamStats* stats) {
    if (stats) *stats = {};
    namespace bd = beam_detail;
    namespace sat = operations_research::sat;
    bd::dc::require(options.width > 0 && options.variants > 0 && options.variants <= 4 && options.completions > 0 &&
        std::isfinite(options.seconds) && options.seconds >= 0 && std::isfinite(options.completion_seconds) && options.completion_seconds > 0 &&
        std::isfinite(options.polish_seconds) && options.polish_seconds >= 0 && options.max_job_tasks >= 0 &&
        std::isfinite(options.raw_completion_share) && options.raw_completion_share > 0 && options.raw_completion_share <= 1 &&
        std::isfinite(options.capacity_seconds) && options.capacity_seconds >= 0 &&
        std::isfinite(options.rebuild_seconds) && options.rebuild_seconds >= 0 &&
        std::isfinite(options.recombine_seconds) && options.recombine_seconds >= 0 &&
        options.dispatch_order >= 0 && options.dispatch_order <= 4 &&
        options.savings_rollouts >= 1 && options.savings_rollouts <= 64 &&
        options.delivery_score >= 0 && options.delivery_score <= 3 && options.first_variant >= 0 && options.first_variant < 4 &&
        (!options.regret_order || options.width == 1) &&
        (!(options.extra_pickups || options.inventory_pickups) || options.terminal_capacity),
        "invalid job beam options");
    const auto start = std::chrono::steady_clock::now();
    auto elapsed = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };
    Result result;
    if (!options.seconds) return result;
    bd::Search search(problem, options.max_job_tasks, options.delivery_score, options.urgent_fragments,
        options.seed_deadlines, options.pickup_deadlines, options.alternate_purchase_scores, options.return_dp);
    search.cache_routes = options.cache_routes;
    auto finish = [&] {
        if (options.cache_routes) result.stages.push_back({"job_beam/cache",
            "hits=" + std::to_string(search.route_cache_hits) + " misses=" + std::to_string(search.route_cache_misses) +
            " entries=" + std::to_string(search.route_cache_size) + " resets=" + std::to_string(search.route_cache_resets), 0});
        result.seconds = elapsed(); return std::move(result);
    };
    std::unordered_set<uint64_t> completed_routes;
    std::map<int, bd::State> rebuild_seeds;
    std::vector<bd::State> route_pool;
    std::vector<int> rebuild_order;
    result.stages.push_back({"job_beam/build", "jobs=" + std::to_string(search.jobs.size()), elapsed()});
    for (int order = 0; order < options.variants * (options.lazy_rebuild ? 2 : 1) && elapsed() < options.seconds; ++order) {
        const bool rebuilding = order >= options.variants;
        const int phase_order = order % options.variants;
        if (rebuilding && !phase_order) {
            for (const auto& [variant, seed] : rebuild_seeds) rebuild_order.push_back(variant);
            std::stable_sort(rebuild_order.begin(), rebuild_order.end(), [&](int a, int b) {
                return rebuild_seeds.at(a).cost < rebuild_seeds.at(b).cost;
            });
        }
        if (rebuilding && phase_order >= int(rebuild_order.size())) break;
        const int variant = rebuilding ? rebuild_order[phase_order] : options.alternate_purchase_scores && options.first_variant == 0 ?
            std::array{0, 2, 1, 3}[phase_order] : (options.first_variant + phase_order) % 4;
        const auto stage = std::string(rebuilding ? "job_beam/retry_v" : "job_beam/v") + std::to_string(variant);
        const double before = elapsed();
        const double search_seconds = std::min(2.0, (options.seconds - elapsed()) / (options.variants - phase_order) / 2);
        const auto deadline = std::chrono::steady_clock::now() +
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(search_seconds));
        auto beam = rebuilding ? std::vector<bd::State>{rebuild_seeds.at(variant)} :
            options.savings_order ? search.merge_portfolio(variant, deadline, options.savings_rollouts) :
            options.dispatch_order >= 3 ? search.dispatch_beam(options.width, variant, deadline, options.dispatch_order == 4, options.dispatch_lookahead) :
            options.dispatch_order ? search.dispatch(variant, deadline, options.dispatch_order == 2, options.dispatch_columns) :
            options.regret_order ? search.regret(variant, deadline) : search.run(options.width, variant, deadline);
        if (!rebuilding) result.stages.push_back({stage + "/search", "depth=" + std::to_string(search.depth) + " candidates=" + std::to_string(search.generated), elapsed() - before});
        if (!rebuilding && options.dispatch_order && options.dispatch_order < 3) result.stages.push_back({stage + "/dispatch",
            "rollouts=" + std::to_string(search.dispatch_rollouts) + " mixed=" + std::to_string(search.dispatch_mixed), 0});
        std::vector<bd::State> polish_history;
        bool raw_first = false;
        bool deferred_polish = false;
        auto polish_first = [&] {
            const double polish_start = elapsed(), old_cost = beam.front().cost;
            const double seconds = std::min(options.polish_seconds, options.seconds - elapsed());
            auto polished = search.polish(beam.front(), variant, std::chrono::steady_clock::now() +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(seconds)),
                options.polish_history ? &polish_history : nullptr, options.polish_chains);
            result.stages.push_back({stage + "/polish", std::to_string(old_cost) + " -> " + std::to_string(polished.cost), elapsed() - polish_start});
            beam.insert(beam.begin() + int(raw_first), std::move(polished));
        };
        if (!rebuilding && !beam.empty() && options.polish_seconds > 0 && elapsed() < options.seconds) {
            raw_first = options.prefer_unpolished &&
                beam.front().cost < 2000 &&  // Below the smallest stock-shortage penalty.
                std::ranges::all_of(beam.front().route_scores, [](const auto& route) { return route.late == 0 && route.length <= 24; });
            deferred_polish = options.defer_polish && raw_first && !options.lazy_rebuild &&
                options.rebuild_seconds == 0 && options.recombine_seconds == 0;
            if (!deferred_polish) polish_first();
        }
        if (options.lazy_rebuild && !rebuilding && !beam.empty()) rebuild_seeds.emplace(variant, beam[int(raw_first)]);
        if ((!options.lazy_rebuild || rebuilding) && !beam.empty() && !search.jobs.empty() && options.rebuild_seconds > 0 && elapsed() < options.seconds) {
            const auto before = elapsed();
            const auto deadline = std::chrono::steady_clock::now() +
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(
                    std::min(options.rebuild_seconds, options.seconds - elapsed())));
            auto alternatives = search.rebuild(beam[int(raw_first)], variant, deadline, options.route_first, options.adaptive_rebuild);
            result.stages.push_back({stage + "/rebuild", "alternatives=" + std::to_string(alternatives.size()), elapsed() - before});
            beam.insert(beam.begin() + int(raw_first), std::make_move_iterator(alternatives.begin()), std::make_move_iterator(alternatives.end()));
        }
        const bool recombining = options.recombine_seconds > 0 && !beam.empty() && !search.jobs.empty();
        if (recombining)
            route_pool.insert(route_pool.end(), beam.begin(), beam.end());
        int ordinary_count = std::min<int>(beam.size(), rebuilding ? 8 : options.completions);
        int completion_count = ordinary_count;
        for (int index = 0; elapsed() < options.seconds; ++index) {
            if (index == 1 && deferred_polish) {
                polish_first();
                ordinary_count = completion_count = std::min<int>(beam.size(), options.completions);
            }
            if (index == ordinary_count && recombining) {
                const double before = elapsed();
                const auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double>(
                        std::min(options.recombine_seconds, options.seconds - elapsed())));
                auto mixed = search.recombine(route_pool, variant, deadline, options.recombine_reassign);
                auto seen = completed_routes;
                for (const auto& state : beam) seen.insert(search.hash(state, true));
                std::erase_if(mixed, [&](const auto& state) { return !seen.insert(search.hash(state, true)).second; });
                result.stages.push_back({stage + "/recombine", "partitions=" + std::to_string(route_pool.size()) +
                    " alternatives=" + std::to_string(mixed.size()), elapsed() - before});
                beam.resize(ordinary_count);
                beam.insert(beam.end(), std::make_move_iterator(mixed.begin()), std::make_move_iterator(mixed.end()));
                completion_count += mixed.size();
            }
            if (index >= completion_count) break;
            const auto& candidate = beam[index];
            if (!completed_routes.insert(search.hash(candidate, true)).second) continue;
            int late = 0, overtime = 0;
            for (const auto& route : candidate.route_scores) { late += route.late; overtime += std::max(0, route.length - 24); }
            if (stats) {
                ++stats->completed_partitions;
                stats->estimated_timing_feasible |= late == 0 && overtime == 0;
            }
            result.stages.push_back({stage + "/candidate" + std::to_string(index), "cost=" + std::to_string(candidate.cost) +
                " overtime=" + std::to_string(overtime) + " late=" + std::to_string(late) +
                " wheat=" + std::to_string(candidate.inputs[kag::WHEAT]) + " fertilizer=" + std::to_string(candidate.inputs[kag::FERTILIZER]), 0});
            if (options.diagnose && !index) result.stages.push_back({stage + "/routes", search.describe(candidate), 0});
            bd::dn::screen::SolveOptions limits;
            limits.seconds = std::min(options.completion_seconds, options.seconds - elapsed()); limits.workers = 1;
            if (deferred_polish && index == 0)
                limits.seconds = std::min(limits.seconds, (options.seconds - elapsed()) * options.raw_completion_share);
            // Preserve successful cheap completions that need their allowance;
            // reserve remaining orderings only when refining a failed schedule.
            const double refinement = std::max(0.0, (options.seconds - elapsed()) / (options.variants - phase_order));
            if (limits.seconds <= 0) break;
            limits.model.dynamic = limits.model.fixed_order = true;
            const auto hint = search.hint(candidate, options.timed_hint || (options.raw_timed_hint && raw_first && index == 0));
            auto completed = options.inventory_pickups ? bd::dn::materialized_screen::solve_capacity_inventory(problem, hint, limits, 2) :
                options.extra_pickups ? bd::dn::materialized_screen::solve_capacity_buffered(problem, hint, limits, 2) :
                options.terminal_capacity ? bd::dn::materialized_screen::solve_capacity_budgeted(problem, hint, limits, 2, refinement)
                : bd::dn::materialized_screen::solve_stock(problem, hint, limits, 2);
            result.stages.push_back({stage + "/complete" + std::to_string(index), sat::CpSolverStatus_Name(completed.status) +
                (completed.materialize_rejection.empty() ? "" : ": " + completed.materialize_rejection), completed.total_seconds});
            result.stages.push_back({stage + "/profile" + std::to_string(index),
                "build=" + std::to_string(completed.build_seconds) + " solve=" + std::to_string(completed.solver_seconds) +
                " materialize=" + std::to_string(completed.materialize_seconds) + " branches=" + std::to_string(completed.branches) +
                " conflicts=" + std::to_string(completed.conflicts), 0});
            if (completed.schedule) { result.schedule = std::move(completed.schedule); return finish(); }
            if (completed.hint && options.capacity_seconds > 0 && elapsed() < options.seconds &&
                problem.start.shed_capacity != storage::unlimited &&
                completed.materialize_rejection.starts_with("strict replay rejected materialization")) {
                bd::dn::exact::HintOptions hints;
                hints.documents[bd::dn::HintKind::fixed_partial] = *completed.hint;
                bd::dn::exact::SolveOptions exact_limits;
                exact_limits.seconds = std::min(options.capacity_seconds, options.seconds - elapsed());
                if (exact_limits.seconds <= 0) return finish();
                exact_limits.workers = 1;
                auto repaired = bd::dn::exact::solve(problem, hints, exact_limits);
                result.stages.push_back({stage + "/capacity_complete" + std::to_string(index), sat::CpSolverStatus_Name(repaired.status),
                    repaired.build_seconds + repaired.solver_seconds});
                if (repaired.replay && repaired.replay->accepted) {
                    result.schedule = std::move(repaired.schedule); return finish();
                }
            }
            if (options.diagnose && !index) {
                for (int diagnostic = 0; diagnostic < 3; ++diagnostic) {
                    limits.model.ignore_availability = diagnostic == 1;
                    limits.model.soft_availability = diagnostic == 2;
                    auto relaxed = bd::dn::materialized_screen::solve(problem, hint, limits, 2);
                    result.stages.push_back({stage + (diagnostic == 2 ? "/soft_delivery" : diagnostic ? "/timing_only" : "/without_stock"), sat::CpSolverStatus_Name(relaxed.status) +
                        (relaxed.materialize_rejection.empty() ? "" : ": " + relaxed.materialize_rejection), relaxed.total_seconds});
                    if (diagnostic == 2) for (const auto& deficit : relaxed.deficits)
                        result.stages.push_back({stage + "/missing_delivery", "item=" + std::to_string(deficit.item) +
                            " hour=" + std::to_string(deficit.deadline) + " quantity=" + std::to_string(deficit.quantity), 0});
                    if (diagnostic == 1 && relaxed.status == sat::INFEASIBLE) {
                        auto seeded = problem;
                        for (int crop = 0; crop < kag::N_CROPS; ++crop) {
                            seeded.start.seeds[crop] += search.data.tasks.size();
                            seeded.end_seeds[crop] += search.data.tasks.size();
                        }
                        const auto without_seeds = bd::dn::materialized_screen::solve(seeded, hint, limits, 2);
                        result.stages.push_back({stage + "/timing_unlimited_seeds", sat::CpSolverStatus_Name(without_seeds.status), without_seeds.total_seconds});
                    }
                }
            }
            if (completed.status == sat::INFEASIBLE && !polish_history.empty()) {
                // A lower routing score can hide a lost seed or delivery slot.
                // Retry the nearest improving incumbent before another beam
                // state; all candidates still use the same completion budget.
                while (!polish_history.empty() && completed_routes.contains(search.hash(polish_history.back(), true)))
                    polish_history.pop_back();
                if (!polish_history.empty()) {
                    beam.insert(beam.begin() + index + 1, std::move(polish_history.back()));
                    polish_history.pop_back();
                    if (index < ordinary_count)
                        completion_count = ordinary_count = std::min<int>(beam.size(), rebuilding ? 8 : options.completions);
                }
            }
        }
    }
    return finish();
}
} // namespace day_scheduler
