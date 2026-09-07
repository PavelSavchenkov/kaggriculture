#include "context.hpp"
#include "task_data.hpp"

namespace day_native::semantic {
day_semantic::Context prepare(const day_solver::DayProblem& problem) {
    using Count = day_solver::InventoryCount;
    const semantic_data::TaskData data(problem);
    day_semantic::Context context;
    std::set<int> inputs, delivery_tasks;
    for (const auto& task : data.tasks) {
        const auto action = task.op == kag::OP_PLACE ? day_semantic::Action::place :
                            task.op == kag::OP_FEED ? day_semantic::Action::feed :
                            task.op == kag::OP_CARE ? day_semantic::Action::care : day_semantic::Action::other;
        context.tasks.push_back({task.tile, task.pattern, task.point[0], task.point[1], task.input, task.output, task.predecessor, action});
        if (task.input >= 0) inputs.insert(task.input);
    }
    for (const auto& [key, tasks] : data.deliveries) for (int task : tasks) {
        context.deadlines[task] = key.second;
        delivery_tasks.insert(task);
    }
    std::vector<const day_solver::MarketEvent*> purchases;
    for (const auto& event : problem.market_plan) purchases.push_back(&event);
    std::sort(purchases.begin(), purchases.end(), [](const auto* a, const auto* b) {
        return std::tie(a->hour, a->order_index) < std::tie(b->hour, b->order_index);
    });
    std::array<Count, kag::N_ITEMS> bought{}, last_supply{};
    for (const auto* event : purchases) {
        if (event->market_op != kag::M_BUY_PRODUCT && event->market_op != kag::M_BUY_ANIMAL) continue;
        if (event->hour <= 21) bought[event->item] += event->quantity;
        else if (event->hour == 22) last_supply[event->item] += event->quantity;
    }
    for (int item : inputs) {
        std::vector<int> targets;
        std::set<int> sources;
        bool produced = false;
        for (const auto& task : data.tasks) {
            if (task.input == item) targets.push_back(task.id);
            if (task.output == item && task.quantity == 1 && !delivery_tasks.contains(task.id)) sources.insert(task.id);
            produced |= task.output == item && task.quantity > 0;
        }
        const Count total = problem.shed_availability.back()[item];
        const Count last = total - problem.shed_availability[22][item];
        const Count free = std::max(Count(0), problem.start.shed[item] + bought[item] - total + std::min(last, last_supply[item]));
        if (free == 0 && sources.size() >= targets.size()) context.groups[item] = {sources, {targets.begin(), targets.end()}};
        // Any route-produced units keep purchase identity fungible. This map
        // only steers the existing late-input neighborhood.
        if (produced) continue;
        std::vector<int> readiness(std::min(Count(targets.size()), problem.start.shed[item]), 0);
        for (const auto* event : purchases) {
            if ((event->market_op != kag::M_BUY_PRODUCT && event->market_op != kag::M_BUY_ANIMAL) || event->item != item || event->hour > 21) continue;
            readiness.insert(readiness.end(), std::min(Count(event->quantity), Count(targets.size() - readiness.size())), event->hour + 1);
        }
        int target = int(targets.size()) - 1;
        for (int ready : readiness) if (ready > 0) context.late_inputs[targets[target--]] = ready;
    }
    return context;
}
} // namespace day_native::semantic
