#pragma once
#include "export.hpp"
#include "../native_constructor_repair/trace.hpp"

namespace day_constructor {
inline json::object proposal_hint(const RouteProposal& proposal) {
    json::array assignments, types;
    for (const auto& value : proposal.assignments) assignments.push_back(json::object{
        {"task", value.task}, {"worker", value.worker}, {"hour", value.rank}, {"type", value.type}});
    for (const auto& group : proposal.type_workers) types.push_back(numbers(group));
    return {{"task_assignments", assignments}, {"type_workers", types}, {"partial_routes", true}};
}
inline json::object proposal_report(const RouteProposal& proposal, RepairOptions options, int workers) {
    const auto original_options = options;
    if (proposal.seed_seconds && *proposal.seed_seconds == 0) options.iterations = 0;
    auto details = repair_report(proposal.repaired, options).at("details").as_object();
    if (proposal.coarse) {
        details["coarse_repair"] = repair_report(*proposal.coarse, original_options).at("details");
        json::array deficits;
        for (const auto& row : proposal.initial_seed_deficits) deficits.push_back(json::object{
            {"crop", row[0]}, {"hour", row[1]}, {"required", row[2]}, {"available", row[3]}, {"shortage", row[4]}});
        details["initial_seed_prefix_deficits"] = deficits;
        details["seed_repair_seconds_allowed"] = *proposal.seed_seconds;
    }
    details["exported_worker_assignment"] = numbers(complete_assignment(proposal.repaired.workers, workers));
    return {{"repaired_lengths", numbers(proposal.repaired.lengths)}, {"repair_violation", numbers(proposal.repaired.score)}, {"repair_details", details}};
}
}  // namespace day_constructor
