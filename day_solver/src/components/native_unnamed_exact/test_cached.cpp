#include "exact.hpp"
#include "../native_cached_exact/exact.hpp"
#include "../native_semantic_control_v2/adapter.hpp"
#include <chrono>
#include <numeric>

int main(int argc, char** argv) {
    using namespace semantic_adapter;
    require(argc == 4, "usage: test_cached problem our_hint output");
    auto supplied = day_solver::load_problem_json(argv[1]);
    const auto original = supplied;
    auto source = read_hint(read_json(argv[2]));
    source.type_workers = std::vector<std::vector<int>>(1);
    source.type_workers->front().resize(original.worker_count);
    std::iota(source.type_workers->front().begin(), source.type_workers->front().end(), 0);
    for (auto& row : source.assignments) row.type = 0;
    const fs::path output = argv[3];
    require(fs::create_directory(output), "use a new output directory");
    cached::Session baseline_session(original);
    unnamed::Session session(supplied);
    supplied.start.shed[0] += 1; // Session must own its input, never borrow caller storage.
    json::array models, timings;
    for (int mode = 0; mode < 6; ++mode) {
        exact::HintOptions hints;
        const auto kind = mode < 3 ? HintKind::fixed_partial : mode == 3 ? HintKind::route_type : HintKind::route_owner;
        hints.documents.emplace(kind, source);
        if (mode == 1) hints.flags.insert(HintFlag::diagnose_partial);
        if (mode == 2 && !source.assignments.empty()) hints.free_tasks.insert(source.assignments.front().task);
        if (mode == 5 && !source.assignments.empty()) hints.free_routes.insert(source.assignments.front().worker);
        exact::SolveOptions settings;
        settings.build_only = settings.include_model = true;
        settings.seconds = 5; settings.workers = 1;
        auto baseline = baseline_session.solve(hints, settings, 2);
        auto candidate = session.solve(hints, settings, 2);
        for (auto* model : {&*baseline.model, &*candidate.model}) {
            for (auto& variable : *model->mutable_variables()) variable.clear_name();
            for (auto& constraint : *model->mutable_constraints()) constraint.clear_name();
        }
        require(baseline.model->SerializeAsString() == candidate.model->SerializeAsString(), "cached model differs");
        require(baseline.restrictions == candidate.restrictions, "restriction metadata differs");
        models.push_back({{"mode", mode}, {"equal", true}, {"baseline_build", baseline.build_seconds},
                          {"cached_build", candidate.build_seconds}, {"variables", baseline.variables}});
        if (mode != 0) continue;
        settings.build_only = settings.include_model = false;
        for (int repeat = 0; repeat < 2; ++repeat) for (int offset = 0; offset < 2; ++offset) {
            const bool use_cache = (repeat + offset) % 2;
            const auto started = std::chrono::steady_clock::now();
            auto result = use_cache ? session.solve(hints, settings, 2) : baseline_session.solve(hints, settings, 2);
            const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
            require(result.schedule && result.replay && result.replay->accepted, "exact control failed replay");
            const auto file = "r" + std::to_string(repeat) + (use_cache ? "_cached.json" : "_baseline.json");
            write_json(output / file, schedule_json(*result.schedule));
            timings.push_back({{"repeat", repeat}, {"cached", use_cache}, {"seconds", seconds},
                               {"build_seconds", result.build_seconds}, {"schedule", file}});
        }
    }
    write_json(output / "report.json", {{"models", models}, {"timings", timings},
               {"preparation_seconds", session.preparation_seconds()}, {"copied_input", true}});
}
