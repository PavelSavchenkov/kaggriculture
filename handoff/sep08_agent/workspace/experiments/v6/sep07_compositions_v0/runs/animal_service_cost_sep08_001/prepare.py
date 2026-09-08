"""Compile cow care exceptions with and without below-parent hiring search."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
original = EXP / 'runs/animal_group_policy_sep08_001'
source = RUN / 'source'
source.mkdir(exist_ok=False)
path = original / 'source/compile_flexible.cpp'
text = path.read_text().replace('#include "season.hpp"',
    '#include "experiments/v6/sep07_compositions_v0/runs/animal_group_policy_sep08_001/source/season.hpp"')
old = 'if(last<=29 && day<last-1)add(OP_CARE);'
new = '''// Isolated28-case engine check proves these two early cow cares
    // exceed the first-yield cap. Recompile all subsequent bank states.
    const bool redundant=r.item==COW && (day==r.first+1 || day==r.first+2);
    if(last<=29 && day<last-1 && !redundant)add(OP_CARE);'''
assert text.count(old) == 1
text = text.replace(old, new)
old = 'for(int hires=0;hires<=2 && !kept;++hires) {'
new = 'for(int hires=SERVICE_MIN_HIRES;hires<=2 && !kept;++hires) {'
assert text.count(old) == 1
text = text.replace(old, new)
old = 'auto trial=problem;auto orders=markets;'
new = '''auto trial=problem;auto orders=markets;
            for(int n=hires;n<0;++n){
                bool removed=false;
                for(int h=23;h>=0 && !removed;--h)for(int s=orders[h].n_orders-1;s>=0;--s){
                    if(orders[h].orders[s].op!=M_HIRE)continue;
                    orders[h].orders[s]={};orders[h].finalize();removed=true;break;
                }
                if(!removed)std::abort();
            }'''
assert text.count(old) == 1
text = text.replace(old, new)
text = text.replace('if(!solved.schedule)solved=fixed_repair(source,trial,seconds);',
    'if(!solved.schedule && hires>=0)solved=fixed_repair(source,trial,seconds);')
(source / 'compile.cpp').write_text(text)
registry = json.loads((EXP / 'configs/league.json').read_text())
sources = set()
for name in ('empty_sale_slots_m2', 'public_router'):
    package = ROOT / registry[name]
    sources.update((package / p).resolve() for p in json.loads((package / 'agent.json').read_text())['sources'])
cmake = '''cmake_minimum_required(VERSION 3.18)
project(animal_service_cost LANGUAGES CXX)
get_filename_component(EXPERIMENT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
get_filename_component(REPOSITORY "${EXPERIMENT}/../../.." ABSOLUTE)
add_subdirectory("${REPOSITORY}/day_solver" day_solver)
add_library(agents OBJECT
'''
cmake += ''.join(f'    "${{REPOSITORY}}/{p.relative_to(ROOT)}"\n' for p in sorted(sources)) + ')\n'
cmake += '''target_compile_features(agents PRIVATE cxx_std_20)
target_include_directories(agents PRIVATE "${REPOSITORY}")
target_compile_options(agents PRIVATE -O3 -march=native -mtune=native)
foreach(target care_only care_minimize)
    add_executable(${target} source/compile.cpp $<TARGET_OBJECTS:agents>)
    target_include_directories(${target} BEFORE PRIVATE "${REPOSITORY}")
    target_include_directories(${target} SYSTEM PRIVATE "${REPOSITORY}/day_solver/vendor/include")
    target_compile_definitions(${target} PRIVATE OR_PROTO_DLL= PROTOBUF_USE_DLLS)
    target_compile_options(${target} PRIVATE -O3 -march=native -mtune=native)
    target_link_libraries(${target} PRIVATE DaySolver::scheduler)
endforeach()
target_compile_definitions(care_only PRIVATE SERVICE_MIN_HIRES=0)
target_compile_definitions(care_minimize PRIVATE SERVICE_MIN_HIRES=-1)
'''
(RUN / 'CMakeLists.txt').write_text(cmake)
cases = []
for seed, folder in [(1014, EXP / 'runs/animal_groups_sep08_001/integrated_audit/cow_triple'),
                     (1019, original / 'contexts_audit/cow_triple_alternate')]:
    baseline = json.loads((folder / 'MATCHED_RESULT.json').read_text())
    spec = RUN / f'cow_triple_{seed}.txt'
    source_spec = original / 'contexts_30s/cow_triple_alternate.txt' if seed == 1019 else EXP / 'runs/animal_groups_sep08_001/later_groups_30s/cow_triple.txt'
    spec.write_text(source_spec.read_text())
    cases.append({'seed': seed, 'spec': str(spec), 'baseline': str(folder.relative_to(EXP)),
        'baseline_result': baseline, 'source_spec': str(source_spec.relative_to(EXP)),
        'source_spec_sha256': hashlib.sha256(source_spec.read_bytes()).hexdigest()})
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'source': str(path.relative_to(EXP)), 'source_sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
    'care_evidence': 'runs/animal_groups_sep08_001/CARE_EQUIVALENCE.json',
    'changes': ['Omit new cow care on birth+1/+2.', 'Rebuild later pending-care states.',
        'Optional one-fewer-than-source worker search; do not apply parent fixed-worker hints to the smaller workforce.'],
    'cases': cases, 'scope': 'Offline whole-course economic/service comparison. No runtime library replacement or promotion.'}, indent=2) + '\n')
print('Prepared care-only and care/minimum-workforce compilers for two audited contexts.')
