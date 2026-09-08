"""Derive the checked compiler with an explicit offline opponent type."""
from pathlib import Path
import hashlib
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
source=EXP/'runs/animal_groups_sep08_001/source'
season=(source/'season_v2.hpp').read_text()
season=season.replace('struct Season {','''#ifdef ANIMAL_COMPILER_PASS
using ContextRival=compositions::Pass;
#else
using ContextRival=compositions::public_router::Agent;
#endif
struct Season {''').replace('public_router::Agent rival;','ContextRival rival;')
(RUN/'source/season.hpp').write_text(season)
compiler=(source/'compile_v8.cpp').read_text().replace('#include "season_v2.hpp"','#include "season.hpp"')
compiler=compiler.replace('#include "fixed_repair.hpp"','#include "experiments/v6/sep07_compositions_v0/runs/animal_groups_sep08_001/source/fixed_repair.hpp"')
compiler=compiler.replace('public_router::Agent rival;','ContextRival rival;')
(RUN/'source/compile.cpp').write_text(compiler)
(RUN/'COMPILER_LINEAGE.json').write_text(json.dumps({'parent':'runs/animal_groups_sep08_001/source/compile_v8.cpp',
    'inputs':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [source/'season_v2.hpp',source/'compile_v8.cpp',source/'fixed_repair.hpp']},
    'changes':['Replace literal public_router opponent with ContextRival in both source extraction and live compilation.',
        'Compile public_router and PASS executables; no change to agent observations, policies, source calendar extraction, or scheduling logic.'],
    'scope':'Offline construction only. Contexts selected by exact public-state entry matches; seeds never enter policy.'},indent=2)+'\n')
