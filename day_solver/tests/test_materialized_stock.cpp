#include "day_solver/scheduler.hpp"
#include "day_solver/io.hpp"
#include "../src/components/native_materialized_screen/screen.hpp"
#include "../src/schedule_hint.hpp"
#include "problem_json.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    namespace ds = day_solver;
    namespace dn = day_native;
    ds::DayProblem problem;
    problem.worker_count = 2;
    ds::ManagedTile animal, crop;
    animal.x = 4; animal.y = 3;
    animal.state.kind = ds::ManagedTileKind::COOP;
    animal.state.animal = kag::GOOSE;
    animal.state.age_days = 4;
    animal.state.fertilizer_available = true;
    crop.x = 5; crop.y = 3;
    crop.state.kind = ds::ManagedTileKind::CROP;
    crop.state.crop = kag::TOMATO;
    crop.state.age_days = 1;
    problem.start.managed_tiles = {animal, crop};
    auto animal_end = animal.state, crop_end = crop.state;
    ++animal_end.age_days; animal_end.stored_units = 1;
    animal_end.consecutive_dry_days = 1;
    ++crop_end.age_days; crop_end.consecutive_dry_days = 1;
    crop_end.fertilizer_days_remaining = 2;
    problem.required_end_tiles = {{0, {}, -1, animal_end}, {1, {}, -1, crop_end}};
    problem.tile_work = {{0, {{kag::OP_COLLECT_FERTILIZER, -1, 1, kag::FERTILIZER, 1}}},
                        {1, {{kag::OP_FERTILIZE, -1, 1, -1, 0}}}};
    ds::MarketEvent hire;
    hire.market_op = kag::M_HIRE; hire.quantity = 1;
    problem.market_plan.push_back(hire);
    day_scheduler::prepare_problem(problem);
    dn::InternalHint hint;
    hint.type_workers = std::vector<std::vector<int>>{{0, 1}};
    hint.assignments = {{0, 0, 1, 0, {}}, {1, 1, 5, 0, {}}};
    dn::screen::SolveOptions limits;
    limits.seconds = 2; limits.workers = 1;
    limits.model.dynamic = limits.model.fixed_order = true;
    const auto solved = dn::materialized_screen::solve_stock(problem, hint, limits, 2);
    if (!solved.schedule) throw std::runtime_error("within-day fertilizer transfer was not constructed: " + solved.materialize_rejection);
    const auto replay = ds::replay_schedule(problem, *solved.schedule);
    if (!replay.candidate.replay.strict_valid || !replay.requirements_satisfied || !replay.invariants_satisfied || !replay.errors.empty())
        throw std::runtime_error("constructed transfer disagrees with physical replay");
    std::cout << "Within-day producer, deposit and recipient pickup passed strict replay.\n";

    // Wheat must remain carried while the single shed slot is used to sell a
    // tomato. Dumping both items loses the tomato before its sale deadline.
    ds::DayProblem selective;
    selective.start.shed_capacity = 1;
    ds::ManagedTile wheat, tomato;
    wheat.x = 4; wheat.y = 4;
    wheat.state.kind = ds::ManagedTileKind::CROP;
    wheat.state.crop = kag::WHEAT; wheat.state.age_days = 2; wheat.state.stored_units = 1;
    tomato = wheat; tomato.y = 3;
    tomato.state.crop = kag::TOMATO; tomato.state.age_days = 8;
    selective.start.managed_tiles = {wheat, tomato};
    ds::ManagedTileState empty;
    auto tomato_end = tomato.state;
    ++tomato_end.age_days; tomato_end.consecutive_dry_days = 1;
    selective.required_end_tiles = {{0, {}, -1, empty}, {1, {}, -1, tomato_end}};
    selective.tile_work = {{0, {{kag::OP_HARVEST, kag::WHEAT, 1, kag::WHEAT, 1}}},
                          {1, {{kag::OP_HARVEST, kag::TOMATO, 1, kag::TOMATO, 1}}}};
    for (int hour = 4; hour < 24; ++hour) selective.shed_availability[hour][kag::TOMATO] = 1;
    selective.end_shed[kag::WHEAT] = 1;
    day_scheduler::prepare_problem(selective);
    dn::InternalHint route;
    route.type_workers = std::vector<std::vector<int>>{{0}};
    route.assignments = {{0, 0, 0, 0, {}}, {1, 0, 1, 0, {}}};
    const auto delivered = dn::materialized_screen::solve_stock(selective, route, limits, 2);
    if (!delivered.schedule) throw std::runtime_error("selective tomato deposit was not constructed: " + delivered.materialize_rejection);
    const auto checked = ds::replay_schedule(selective, *delivered.schedule);
    if (!checked.candidate.replay.strict_valid || !checked.requirements_satisfied || !checked.invariants_satisfied || !checked.errors.empty())
        throw std::runtime_error("selective deposit disagrees with physical replay");
    std::cout << "Selective sale deposit preserved unrelated carried wheat.\n";

    // Two workers produce two items for one shed slot. The requested tomato
    // must survive automatic worker/cargo-ordered night deposits; discarding
    // the wheat is part of this contract, not a missing delivery.
    auto night = selective;
    night.worker_count = 2;
    night.market_plan.push_back(hire);
    night.shed_availability = {};
    night.end_shed = {};
    night.end_shed[kag::TOMATO] = 1;
    day_scheduler::prepare_problem(night);
    dn::InternalHint night_routes;
    night_routes.type_workers = std::vector<std::vector<int>>{{0, 1}};
    night_routes.assignments = {{0, 0, 0, 0, {}}, {1, 1, 0, 0, {}}};
    const auto retained = dn::materialized_screen::solve_capacity(night, night_routes, limits, 2);
    if (!retained.schedule) throw std::runtime_error("night capacity was not constructed: " + retained.materialize_rejection);
    const auto night_check = ds::replay_schedule(night, *retained.schedule);
    if (!night_check.candidate.replay.strict_valid || !night_check.requirements_satisfied ||
        !night_check.invariants_satisfied || !night_check.errors.empty())
        throw std::runtime_error("night capacity disagrees with physical replay");
    std::cout << "Night capacity retained the required item and discarded surplus.\n";

    // Picking only the wheat consumed by feeding leaves too little room for
    // the fixed fertilizer purchase. Spare wheat may stay carried until night.
    ds::DayProblem buffer;
    buffer.start.shed_capacity = 4;
    buffer.start.shed[kag::WHEAT] = 4;
    buffer.start.managed_tiles = {animal, crop};
    auto fed_end = animal_end;
    fed_end.consecutive_dry_days = 0;
    buffer.required_end_tiles = {{0, {}, -1, fed_end}, {1, {}, -1, crop_end}};
    buffer.tile_work = {{0, {{kag::OP_FEED, -1, 1, -1, 0}}},
                       {1, {{kag::OP_FERTILIZE, -1, 1, -1, 0}}}};
    ds::MarketEvent fertilizer;
    fertilizer.hour = 1; fertilizer.market_op = kag::M_BUY_PRODUCT;
    fertilizer.item = kag::FERTILIZER; fertilizer.quantity = 2;
    buffer.market_plan = {fertilizer};
    buffer.end_shed[kag::WHEAT] = 3;
    buffer.end_shed[kag::FERTILIZER] = 1;
    day_scheduler::prepare_problem(buffer);
    dn::InternalHint buffer_route;
    buffer_route.type_workers = std::vector<std::vector<int>>{{0}};
    buffer_route.assignments = {{0, 0, 4, 0, {}}, {1, 0, 7, 0, {}}};
    const auto buffered = dn::materialized_screen::solve_capacity_buffered(buffer, buffer_route, limits, 2);
    if (!buffered.schedule) throw std::runtime_error("purchase space was not constructed: " + buffered.materialize_rejection);
    const auto buffer_check = ds::replay_schedule(buffer, *buffered.schedule);
    if (!buffer_check.candidate.replay.strict_valid || !buffer_check.requirements_satisfied ||
        !buffer_check.invariants_satisfied || !buffer_check.errors.empty())
        throw std::runtime_error("extra pickup disagrees with physical replay");
    std::cout << "Extra wheat pickup preserved fertilizer purchase and exact end stock.\n";

    // Selling newly harvested melons does not remove wheat already stored.
    // Aggregate buys minus sales hides the spare pickup needed on this day.
    ds::DayProblem mixed;
    mixed.worker_count = 2;
    mixed.start.shed_capacity = 4; mixed.start.shed[kag::WHEAT] = 4;
    auto melon = wheat;
    melon.x = 5; melon.y = 4; melon.state.crop = kag::MELON;
    melon.state.age_days = 10; melon.state.stored_units = 2;
    mixed.start.managed_tiles = {animal, melon};
    mixed.required_end_tiles = {{0, {}, -1, fed_end}, {1, {}, -1, empty}};
    mixed.tile_work = {{0, {{kag::OP_FEED, -1, 1, -1, 0}}},
                      {1, {{kag::OP_HARVEST, kag::MELON, 1, kag::MELON, 2}}}};
    auto wheat_buy = fertilizer;
    wheat_buy.hour = 3; wheat_buy.item = kag::WHEAT;
    mixed.market_plan = {hire, wheat_buy};
    for (int hour = 3; hour < 24; ++hour) mixed.shed_availability[hour][kag::MELON] = 2;
    mixed.end_shed[kag::WHEAT] = 4;
    day_scheduler::prepare_problem(mixed);
    dn::InternalHint mixed_routes;
    mixed_routes.type_workers = std::vector<std::vector<int>>{{0, 1}};
    mixed_routes.assignments = {{0, 0, 2, 0, {}}, {1, 1, 1, 0, {}}};
    const auto mixed_result = dn::materialized_screen::solve_capacity(mixed, mixed_routes, limits, 2);
    if (!mixed_result.schedule) throw std::runtime_error("sales of produced goods hid purchase pressure");
    const auto mixed_check = ds::replay_schedule(mixed, *mixed_result.schedule);
    if (!mixed_check.candidate.replay.strict_valid || !mixed_check.requirements_satisfied || !mixed_check.invariants_satisfied)
        throw std::runtime_error("mixed-item purchase pressure disagrees with replay");
    std::cout << "Produced melon sales preserved wheat purchase space.\n";

    if (argc < 3 || !(argc % 2)) throw std::runtime_error("supply diagnostic materialization problem/action pairs");
    for (int file = 1; file < argc; file += 2) {
    const auto contract = ds::load_problem_json(argv[file]);
    std::ifstream input(argv[file + 1]);
    std::array<kag::Action, ds::HOURS> witness;
    for (auto& action : witness) {
        input >> action.n_units >> action.n_orders;
        for (int worker = 0; worker < action.n_units; ++worker) {
            int op, arg;
            input >> op >> arg >> action.units[worker].n;
            action.units[worker].op = op; action.units[worker].arg = arg;
        }
        for (int slot = 0; slot < action.n_orders; ++slot) {
            int op, item;
            input >> op >> item >> action.orders[slot].n;
            action.orders[slot].op = op; action.orders[slot].item = item;
        }
        action.finalize();
    }
    if (!input) throw std::runtime_error("cannot read materialization witness");
    const auto source = ds::replay_schedule(contract, witness);
    if (!source.candidate.replay.strict_valid || !source.requirements_satisfied || !source.invariants_satisfied)
        throw std::runtime_error("invalid materialization witness");
    limits.seconds = 15;
    const auto diagnostic = dn::materialized_screen::solve_capacity_ordered(contract,
        day_scheduler::schedule_hint(contract, witness), limits, 2);
    if (!diagnostic.schedule) throw std::runtime_error(std::string(argv[file]) + ": " + diagnostic.materialize_rejection);
    const auto validated = ds::replay_schedule(contract, *diagnostic.schedule);
    if (!validated.candidate.replay.strict_valid || !validated.requirements_satisfied || !validated.invariants_satisfied)
        throw std::runtime_error("DROP input regression failed physical replay");
    std::cout << "Diagnostic route order preserved inputs and retained cargo: " << argv[file] << '\n';
    }
}
