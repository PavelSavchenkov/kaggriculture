from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
source = (EXP / 'include/crop_source_season.hpp').read_text()
start = source.index('#include "day_compile_helpers.hpp"')
end = source.index('struct CropLife')
source = source[:start] + '''#include "experiments/v6/sep07_compositions_v0/include/day_compile_helpers.hpp"
#include "experiments/v6/sep07_compositions_v0/include/biology.hpp"
#include "experiments/v6/sep07_compositions_v0/include/crop_branch_sequence.hpp"
#include "experiments/v6/sep07_compositions_v0/runs/opening_market_search_001/proposals/opening_q32_b13_v1/source/agent.hpp"

using namespace compositions;
using namespace compositions::day_contract;
#ifndef LATE_BERRY
#define LATE_BERRY 0
#endif
// Offline leaf source. The real opening and day-12 decision still execute.
// Only an already valid wheat suffix may enter this forced berry continuation.
class Source : public CropBranchSequence<opening_q32_b13_v1::Agent> {
public:
    Source() : CropBranchSequence(wheat_one_fert::off_days(), wheat_one_fert::on_days(), 20, LATE_BERRY ? 0 : 1000) {}
    static kag::agent::AgentInfo info() { return {"late_animal_source"}; }
};

''' + source[end:]
(RUN / 'source/season.hpp').write_text(source)
original = EXP / 'src/compile_crop_rotation.cpp'
(RUN / 'SOURCE_LINEAGE.json').write_text(json.dumps({
    'base': 'runs/opening_market_search_001/proposals/opening_q32_b13_v1',
    'base_scope': 'Fixed q32 candidate; promotion still under final review when run begins.',
    'season_extractor': {'path': 'include/crop_source_season.hpp', 'sha256': hashlib.sha256((EXP / 'include/crop_source_season.hpp').read_bytes()).hexdigest()},
    'compiler_donor': {'path': 'src/compile_crop_rotation.cpp', 'sha256': hashlib.sha256(original.read_bytes()).hexdigest()},
    'source_course': 'wheat_one_fert off/on immutable schedules, forced only for offline continuation extraction; existing physical guards remain.',
    'idea': 'Original user requested general animal/wait choice and multi-investment composition changes; a released crop tile may support a second later investment. No new external policy blocks.'
}, indent=2) + '\n')
