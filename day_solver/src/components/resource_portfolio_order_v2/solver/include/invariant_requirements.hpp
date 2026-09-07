#pragma once

#include <string>
#include <vector>

#include "day_solver_api.hpp"

namespace day_solver {

// Carrots at age 2 may either remain the same crop for one more day or be
// harvested now. This is the only v1 invariant that is disjunctive.
struct PreserveOrHarvest {
    int16_t tile = NO_TILE;
    int16_t crop = NO_SUBJECT;
    int16_t harvest_units = 0;
};

struct InvariantRequirements {
    std::vector<OutcomeBound> outcomes;
    std::vector<EndTileRequirement> end_tiles;
    std::vector<PreserveOrHarvest> alternatives;
    std::vector<std::string> errors;
};

InvariantRequirements derive_invariant_requirements(
    const PhysicalState& start);
InvariantRequirements derive_invariant_requirements(const DayProblem& problem);

}  // namespace day_solver
