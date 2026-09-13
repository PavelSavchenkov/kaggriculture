#include "day_solver/scheduler.hpp"
#include "day_solver/io.hpp"
#include "../src/storage.hpp"
#include "../src/fixed_path_stock.hpp"
#include "../src/schedule_hint.hpp"
#include "../src/components/native_solver_api/exact.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace kag;
namespace ds = day_solver;

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
static bool valid(const ds::DayProblem& problem, const std::array<Action, 24>& actions) {
    const auto result = ds::replay_schedule(problem, actions);
    return result.requirements_satisfied && result.invariants_satisfied &&
           result.candidate.replay.strict_valid && result.errors.empty();
}
static std::array<Action, 24> empty_schedule() {
    std::array<Action, 24> result;
    for (auto& action : result) { action.n_units = 1; action.finalize(); }
    return result;
}
static std::array<Action, 24> load_actions(const std::filesystem::path& path) {
    std::ifstream in(path);
    std::array<Action, 24> result;
    for (auto& action : result) {
        in >> action.n_units >> action.n_orders;
        for (int worker = 0; worker < action.n_units; ++worker) {
            int op, arg;
            in >> op >> arg >> action.units[worker].n;
            action.units[worker].op = op; action.units[worker].arg = arg;
        }
        for (int slot = 0; slot < action.n_orders; ++slot) {
            int op, item;
            in >> op >> item >> action.orders[slot].n;
            action.orders[slot].op = op; action.orders[slot].item = item;
        }
        action.finalize();
    }
    require(bool(in), "cannot read storage regression actions");
    return result;
}

int main(int argc, char** argv) {
    require(argc == 3, "supply storage fixture directory and output actions path");
    // A partial PLACE leaves excess cargo carried; automatic night transfer
    // discards it when the shed remains full. Purchases require real headroom.
    ds::DayProblem partial;
    partial.start.shed_capacity = 2;
    partial.start.shed[MILK] = 2;
    partial.end_shed[MILK] = partial.end_shed[WHEAT] = 1;
    ds::MarketEvent buy;
    buy.hour = 0; buy.order_index = 0; buy.market_op = M_BUY_PRODUCT;
    buy.item = WHEAT; buy.quantity = 1;
    partial.market_plan.push_back(buy);
    day_scheduler::prepare_problem(partial);
    auto actions = empty_schedule();
    actions[0].units[0] = {OP_PICKUP, MILK, 2};
    actions[0].n_orders = 1; actions[0].orders[0] = {M_BUY_PRODUCT, WHEAT, 1};
    actions[1].units[0] = {OP_PLACE, MILK, 2};
    for (auto& action : actions) action.finalize();
    require(valid(partial, actions), "partial PLACE or night overflow is wrong");
    actions[1].units[0] = {OP_DROP, 0, 1}; actions[1].finalize();
    require(valid(partial, actions), "DROP overflow is wrong");
    actions[0].units[0] = {}; actions[0].finalize();
    require(!valid(partial, actions), "purchase into full shed was accepted");

    // Repair the quantity on an existing pickup, without adding a visit. One
    // unit is too little to make room for the fixed two-unit purchase.
    auto purchase_space = partial;
    purchase_space.start.shed_capacity = 4;
    purchase_space.start.shed[MILK] = 4;
    purchase_space.end_shed[MILK] = purchase_space.end_shed[WHEAT] = 2;
    purchase_space.market_plan[0].quantity = 2;
    day_scheduler::prepare_problem(purchase_space);
    auto too_small = empty_schedule();
    too_small[0].units[0] = {OP_PICKUP, MILK, 1};
    too_small[0].n_orders = 1; too_small[0].orders[0] = {M_BUY_PRODUCT, WHEAT, 2};
    too_small[0].finalize();
    require(!valid(purchase_space, too_small), "undersized pickup left enough purchase space");
    const auto enlarged = day_scheduler::storage::repair(purchase_space, too_small,
        std::chrono::steady_clock::now() + std::chrono::milliseconds(50));
    require(enlarged && valid(purchase_space, *enlarged), "existing pickup did not clear purchase space");
    require((*enlarged)[0].units[0].n == 2, "pickup repair changed the wrong visit");

    // Night transfer follows workers' cargo insertion order.
    ds::DayProblem ordered;
    ordered.start.shed_capacity = 2;
    ordered.start.shed[MILK] = ordered.start.shed[STRAWBERRY] = 1;
    ordered.end_shed[MILK] = ordered.end_shed[WHEAT] = 1;
    buy.hour = 1; ordered.market_plan.push_back(buy);
    day_scheduler::prepare_problem(ordered);
    actions = empty_schedule();
    actions[0].units[0] = {OP_PICKUP, MILK, 1};
    actions[1].units[0] = {OP_PICKUP, STRAWBERRY, 1};
    actions[1].n_orders = 1; actions[1].orders[0] = {M_BUY_PRODUCT, WHEAT, 1};
    for (auto& action : actions) action.finalize();
    require(valid(ordered, actions), "night cargo order lost the milk");
    std::swap(actions[0].units[0], actions[1].units[0]);
    actions[0].finalize(); actions[1].finalize();
    require(!valid(ordered, actions), "wrong night cargo order was accepted");

    for (const auto* problem : {&partial, &ordered}) {
        const auto solved = day_native::exact::solve(*problem, {}, {2, 1});
        require(solved.schedule && solved.replay && solved.replay->accepted,
                "capacity model could not solve synthetic storage case");
        require(valid(*problem, *solved.schedule), "capacity model disagrees with strict replay");
    }

    const std::filesystem::path folder(argv[1]);
    // A cold proposal fills the shed before its later carrot deposits. Repair
    // transfer quantities and use idle shed visits while keeping every path
    // and farm work time fixed.
    const auto crowded = ds::load_problem_json(folder / "full_shed_day28.json");
    const auto crowded_actions = load_actions(folder / "full_shed_day28_proposal.actions");
    require(!valid(crowded, crowded_actions), "crowded shed proposal unexpectedly valid");
    const auto stocked = fixed_path_stock::solve(crowded, crowded_actions,
        day_scheduler::schedule_hint(crowded, crowded_actions), 1, true);
    require(stocked.schedule && valid(crowded, *stocked.schedule), "fixed-path stock repair missed daytime capacity");
    const auto problem = ds::load_problem_json(folder / "overflow_day.json");
    require(problem.start.shed_capacity == 100, "JSON omitted capacity");
    const auto roundtrip = ds::parse_problem_json(ds::serialize_problem_json(problem));
    require(roundtrip.start.shed_capacity == 100, "JSON round trip lost capacity");
    auto malformed = ds::serialize_problem_json(problem);
    const auto field = malformed.find("\"shed_capacity\":100");
    require(field != std::string::npos, "capacity field missing from serialized fixture");
    malformed.replace(field, std::string("\"shed_capacity\":100").size(), "\"shed_capacity\":32767");
    bool rejected = false;
    try { ds::parse_problem_json(malformed); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected, "reserved unlimited sentinel accepted as explicit capacity");
    require(valid(problem, load_actions(folder / "overflow_source.actions")), "source overflow witness failed");
    require(!valid(problem, load_actions(folder / "overflow_wrong_goods.actions")), "lost milk schedule was accepted");
    const auto result = day_scheduler::solve(problem, {5, 1});
    for (const auto& stage : result.stages)
        std::cerr << stage.name << ' ' << stage.status << ' ' << stage.seconds << '\n';
    require(result.schedule.has_value(), "capacity solver did not repair overflow day");
    require(valid(problem, *result.schedule), "capacity solver returned invalid day");
    std::ofstream out(argv[2]);
    require(bool(out), "cannot write storage regression schedule");
    for (const auto& action : *result.schedule) {
        out << action.n_units << ' ' << action.n_orders;
        for (int worker = 0; worker < action.n_units; ++worker)
            out << ' ' << +action.units[worker].op << ' ' << +action.units[worker].arg << ' ' << action.units[worker].n;
        for (int slot = 0; slot < action.n_orders; ++slot)
            out << ' ' << +action.orders[slot].op << ' ' << +action.orders[slot].item << ' ' << action.orders[slot].n;
        out << '\n';
    }
    std::cout << "storage controls passed; real day solved in " << result.seconds << " seconds\n";
}
