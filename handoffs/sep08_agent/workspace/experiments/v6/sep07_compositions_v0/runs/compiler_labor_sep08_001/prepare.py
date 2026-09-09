"""Freeze a hiring-only compiler comparison; keep every earlier policy intact."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
PARENT = EXP / 'candidates/composition_greedy_v0'


def replace(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


core = RUN / 'compiler'
(core / 'source').mkdir(parents=True, exist_ok=False)
header = (PARENT / 'source/agent.hpp').read_text()
header = header.replace('namespace compositions::greedy {', 'namespace compositions::compiler_labor_sep08 {')
header = header.replace('../../../include/composition.hpp', '../../../../include/composition.hpp')
header = replace(header, 'int reserve_days=0,bool flexible_hiring=false)', 'int reserve_days=0,bool flexible_hiring=false,int labor_mode=0)')
header = replace(header, 'source_service_(source_service),flexible_hiring_(flexible_hiring) {}',
                 'source_service_(source_service),flexible_hiring_(flexible_hiring),labor_mode_(labor_mode) {}')
header = replace(header, 'int reserve_days=1,bool flexible_hiring=true)', 'int reserve_days=1,bool flexible_hiring=true,int labor_mode=0)')
header = replace(header, 'flexible_hiring_(flexible_hiring),custom_(true)', 'flexible_hiring_(flexible_hiring),labor_mode_(labor_mode),custom_(true)')
header = replace(header, '    bool custom_=false;', '    int labor_mode_=0;\n    std::array<int,30> estimated_hands_{};\n    bool custom_=false;')
(core / 'source/agent.hpp').write_text(header)
source = (PARENT / 'source/agent.cpp').read_text()
source = source.replace('namespace compositions::greedy {', 'namespace compositions::compiler_labor_sep08 {')
source = replace(source, '#include "agent.hpp"', '#include "agent.hpp"\n#include "../../../../include/estimate.hpp"')
source = replace(source, '#include "programs.inc"', '#include "../../../../candidates/composition_greedy_v0/source/programs.inc"')
source = replace(source, '\n}\n\nvoid AgentCore::act(', '''
    if(labor_mode_<0 || labor_mode_>3)std::abort();
    estimated_hands_.fill(0);
    if(labor_mode_==3) {
        EstimateOptions options;
        options.recorded_layout=true;
        options.service=source_service_?ServiceModel::Recorded:ServiceModel::Productive;
        estimated_hands_=estimate_plan(intents_,support_,options).hands;
    }
}

void AgentCore::act(''')
source = replace(source, '        if(o.hour<=3)for(int n=farm.hires_today;', '''        if(labor_mode_==1)target_hands=8;
        if(labor_mode_==2)target_hands=10;
        if(labor_mode_==3)target_hands=std::max(target_hands,estimated_hands_[o.day]);
        if(o.hour<=3)for(int n=farm.hires_today;''')
source = replace(source, '        for(int n=farm.hires_today;n<target;++n)', '''        if(labor_mode_==1)target=8;
        if(labor_mode_==2)target=10;
        if(labor_mode_==3)target=std::max(target,estimated_hands_[o.day]);
        for(int n=farm.hires_today;n<target;++n)''')
(core / 'source/agent.cpp').write_text(source)
(core / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': 'compiler_labor_sep08_core',
    'header': 'source/agent.hpp', 'type': 'compositions::compiler_labor_sep08::Agent<>', 'sources': ['source/agent.cpp']}, indent=2)+'\n')
(core / 'README.md').write_text('# Hiring-only compiler experiment\n\nCopied composition_greedy_v0 with four hiring modes: unchanged, eight hands, ten hands, or the existing dated-composition estimator as a minimum. Placement, service, job assignment, funding order and sales are unchanged. No strength claim. Parent hashes and replay sources are in ../LINEAGE.json.\n')

catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for case in ['mixed', 'goose', 'p4', 'p55']:
    for mode in range(4):
        name = f'compiler_labor_{case}_m{mode}'
        folder = RUN / 'proposals' / name
        (folder / 'source').mkdir(parents=True, exist_ok=False)
        if case.startswith('p'):
            constructor = f'Agent():AgentCore({int(case[1:])},true,false,true,1,true,{mode}) {{}}'
        else:
            counts = '2,2,0,7,12,8' if case == 'mixed' else '0,0,6,12,0,0'
            constructor = f'Agent():AgentCore(compiler_labor_data::cold_farm({counts}),Support{{}},false,false,false,1,true,{mode}) {{}}'
        code = f'''#pragma once
#include "../../../compiler/source/agent.hpp"
#include "../../../cold_farm.hpp"
namespace compositions::{name} {{
class Agent:public compiler_labor_sep08::AgentCore {{
public:
    {constructor}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
'''
        (folder / 'source/agent.hpp').write_text(code)
        (folder / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
            'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../compiler/source/agent.cpp']}, indent=2)+'\n')
        (folder / 'README.md').write_text(f'# {name}\n\nExperimental dated-composition compiler, case {case}, hiring mode {mode}. See ../../LINEAGE.json and ../../README.md for exact origins, controls and evaluation scope. Not promoted or submitted.\n')
        assert name not in catalog
        catalog[name] = str(folder.relative_to(ROOT))
        names.append(name)

cold = (EXP / 'src/cold_compiler.cpp').read_text()
start = cold.index('std::vector<Life> cold_farm(')
end = cold.index('\nint main(', start)
(RUN / 'cold_farm.hpp').write_text('#pragma once\n#include <algorithm>\n#include "../../include/composition.hpp"\nnamespace compositions::compiler_labor_data {\n'+'inline '+cold[start:end]+'\n}\n')
catalog_path.write_text(json.dumps(catalog, indent=2)+'\n')
lineage = {'created_utc': datetime.now(timezone.utc).isoformat(), 'parent': str(PARENT.relative_to(EXP)),
    'parent_sha256': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in [PARENT/'source/agent.hpp', PARENT/'source/agent.cpp', PARENT/'source/programs.inc', PARENT/'PROGRAM_SOURCES.json', EXP/'src/cold_compiler.cpp', EXP/'include/estimate.hpp']},
    'variants': names, 'hypothesis': 'Pending jobs omit future construction and service. Cheap hiring formula may underfund worker time. Test that causal factor before changing routes.',
    'mode_0': 'Exact unchanged compiler with the same case flags; source support is disabled in all four comparison modes.',
    'mode_1': 'Eight daily hands, subject to existing funding/order constraints.',
    'mode_2': 'Ten daily hands, subject to the same constraints.',
    'mode_3': 'Existing estimate_plan daily hands as a minimum; no refitting yet.',
    'discovery': {'seed_start': 1000, 'seeds': 8, 'seat_mode': 'both', 'opponents': ['public_router', 'observed_sale_lead_start_216']},
    'scope': 'Compiler diagnosis on four fixed compositions; no promotion, no independent audit use.'}
(RUN / 'LINEAGE.json').write_text(json.dumps(lineage, indent=2)+'\n')
print('Prepared',len(names),'hiring-only policies.')
