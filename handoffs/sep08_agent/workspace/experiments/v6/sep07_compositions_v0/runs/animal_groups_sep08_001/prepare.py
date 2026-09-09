"""Prepare an offline group-investment compiler against the frozen current agent."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
OLD = EXP/'runs/late_animal_rotation_001'
reference = json.loads((EXP/'CURRENT_REFERENCE.json').read_text())
assert reference['name'] == 'empty_sale_slots_m2'
parent = EXP/reference['path']
manifest = json.loads((parent/'agent.json').read_text())
source = RUN/'source'
source.mkdir(exist_ok=True)
assert not (source/'season.hpp').exists()

season = (OLD/'source/season.hpp').read_text()
begin = season.index('#include "experiments/v6/sep07_compositions_v0/include/crop_branch_sequence.hpp"')
end = season.index('struct CropLife')
season = season[:begin] + f'''#include "{(parent/manifest['header']).relative_to(ROOT)}"
using namespace compositions;
using namespace compositions::day_contract;
using Source={manifest['type']};
inline kag::agent::DecisionBudget decision_budget(){{
    kag::agent::DecisionBudget b;b.max_expansions=100000;return b;
}}

''' + season[end:]
season = season.replace('std::vector<RecordedDay> days;', '''std::vector<RecordedDay> days;
    std::array<Farm,2> final_farms;
    uint64_t hashes[2]={14695981039346656037ULL,14695981039346656037ULL};''')
season = season.replace(',{},pair[', ',decision_budget(),pair[')
season = season.replace('raw[sim.st.day][sim.st.hour]=pair[0];', '''for(int p=0;p<2;++p){
                validate_action(pair[p],agent::runtime::make_observation(sim,p));
                hash_action(hashes[p],pair[p]);
            }
            raw[sim.st.day][sim.st.hour]=pair[0];''')
season = season.replace('        // Pad only', '''        if(sim.st.step!=719)std::abort();
        final_farms={sim.st.farms[0],sim.st.farms[1]};
        // Pad only''')
(source/'season.hpp').write_text(season)

compiler = (OLD/'source/compile.cpp').read_text()
compiler = compiler.replace('#include "sequence.hpp"', '// Offline fixtures only: no cross-scenario policy is claimed here.')
compiler = compiler.replace('if(r.item<0)return original;', 'if(r.item<0 || day<r.first)return original;')
start = compiler.index('    if(argc!=7)return 2;')
end = compiler.index('    const bool cancel_unused_seeds', start)
compiler = compiler[:start] + '''    if(argc!=5)return 2;
    const fs::path directory=fs::absolute(argv[1]);
    const uint64_t seed=std::stoull(argv[2]);
    const double seconds=std::stod(argv[4]);
    std::ifstream spec(argv[3]);if(!spec || fs::exists(directory) || seconds<=0)return 2;
    std::vector<Rotation> rotations;Rotation r;int first=30;std::array<bool,100> used{};
    while(spec>>r.cell>>r.first>>r.item){
        if(r.cell<0 || r.cell>=100 || r.first<8 || r.first>22 || !is_animal(r.item) || used[r.cell])return 2;
        used[r.cell]=true;first=std::min(first,r.first);rotations.push_back(r);
    }
    if(!spec.eof() || rotations.empty())return 2;
''' + compiler[end:]
compiler = compiler.replace(',{},pair[', ',decision_budget(),pair[')
compiler = compiler.replace('        for(const auto& r:rotations) {', '        for(const auto& r:rotations) {\n            if(day<r.first)continue;')
start = compiler.index('    Options options;options.a=name;')
compiler = compiler[:start] + '''    std::ofstream result(directory/"MATCHED_RESULT.json");
    result<<"{\\"scope\\":\\"offline fixed-calendar matched scenario, not deployable policy evaluation\\","
          <<"\\"seed\\":"<<seed<<",\\"parent_cash\\":"<<season.final_farms[0].money
          <<",\\"parent_rival_cash\\":"<<season.final_farms[1].money
          <<",\\"cash\\":"<<sim.st.farms[0].money<<",\\"rival_cash\\":"<<sim.st.farms[1].money;
    for(int p=0;p<2;++p){
        result<<",\\"produced"<<p<<"\\":[";
        for(int i=0;i<N_ITEMS;++i)result<<(i?",":"")<<sim.st.farms[p].produced[i];
        result<<"],\\"parent_produced"<<p<<"\\":[";
        for(int i=0;i<N_ITEMS;++i)result<<(i?",":"")<<season.final_farms[p].produced[i];
        result<<"]";
    }
    result<<"}\\n";
    std::ofstream(directory/"STATUS.json")<<"{\\"status\\":\\"compiled\\",\\"days\\":"<<sequence.size()<<"}\\n";
}
'''
(source/'compile.cpp').write_text(compiler)

translations = [(parent/p).resolve() for p in manifest['sources']]
translations += [EXP/'league/top_replay_library/source/agent.cpp',EXP/'league/public_router/source/agent.cpp']
cmake = '''cmake_minimum_required(VERSION 3.18)
project(animal_groups LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
foreach(target estimate compile)
    add_executable(${target} source/${target}.cpp
'''
cmake += ''.join(f'        "${{REPOSITORY}}/{p.relative_to(ROOT)}"\n' for p in translations)
cmake += '''    )
    target_include_directories(${target} BEFORE PRIVATE "${REPOSITORY}")
    target_compile_options(${target} PRIVATE -O3 -march=native -mtune=native)
    target_link_libraries(${target} PRIVATE DaySolver::scheduler)
endforeach()
'''
(RUN/'CMakeLists.txt').write_text(cmake)
(RUN/'LINEAGE.json').write_text(json.dumps({
    'created_utc':datetime.now(timezone.utc).isoformat(),'parent':reference,
    'copied_sources':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in [OLD/'source/season.hpp',OLD/'source/compile.cpp']},
    'changes':['Extract actual strongest-agent calendars at the normal 100000 expansion budget.',
        'Compile multiple dated animal additions; preserve original jobs before each addition.',
        'Retain solver certificates and full live-opponent physical endpoint checks.',
        'Remove old unguarded cross-scenario sequence evaluation: semantic branch handling remains required.'],
    'limitations':['Source calendar is retrospective and can contain future branch choices.',
        'Initial estimator omits added labor cost; its score is an optimistic screen, not expected realized profit.',
        'Fixed recorded trades can lose the parent market policy adaptivity.',
        'No new deployable agent or promotion is claimed by this offline study.']},indent=2)+'\n')
print('Prepared current-source extraction and dated group compiler.',flush=True)
