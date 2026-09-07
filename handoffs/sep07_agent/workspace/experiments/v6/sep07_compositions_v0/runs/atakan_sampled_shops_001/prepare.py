"""Freeze Atakan courses and generate integration-only C++ policy packages."""
import hashlib
import json
from pathlib import Path

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
SOURCE = RUN.parent / 'atakan_portfolio_001'
COUNTS = [0, 1, 8, 32, 64]
NAMES = [f'atakan_integrated_s{count}_{objective}' for count in COUNTS for objective in ['own', 'margin']]


def main():
    (RUN / 'source').mkdir(parents=True, exist_ok=True)
    data = (SOURCE / 'source/data.inc').read_bytes()
    (RUN / 'source/data.inc').write_bytes(data)
    upstream = json.loads((SOURCE / 'LINEAGE.json').read_text())
    sampler = EXP / 'include/sampled_animal_value.hpp'
    lineage = {'donor_lineage': upstream, 'source_run': str(SOURCE.relative_to(EXP)),
               'copied_course_data_sha256': hashlib.sha256(data).hexdigest(),
               'sampling_design_source': str(sampler.relative_to(EXP)), 'sampling_design_sha256': hashlib.sha256(sampler.read_bytes()).hexdigest(),
               'change': 'Unknown-shop demand only: original35% mean, full mean, or8/32/64 shared stratified shop sequences. All other Atakan formulas and actions retained.',
               'preserved': ['Own donor dated successful flows and fixed costs', 'Rival current-herd forecast', 'Daily midpoint prices, internal netting and price-floor handling',
                             'Current-day fraction and original town-center demand approximation', 'Complete719-action course and decision step226'],
               'observations': 'Sampler reads only current observation; unknown sequences use fixed local constants, never environment seed or future observations.'}
    (RUN / 'LINEAGE.json').write_text(json.dumps(lineage, indent=2)+'\n')
    for count in COUNTS:
        for objective in ['own', 'margin']:
            name = f'atakan_integrated_s{count}_{objective}'
            package = RUN / 'proposals' / name
            (package / 'source').mkdir(parents=True, exist_ok=True)
            (package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/agent.hpp"\n'
                f'namespace compositions::{name} {{ class Agent:public atakan_integrated::Agent {{public:Agent():atakan_integrated::Agent({count},{str(objective == "margin").lower()}){{}} static kag::agent::AgentInfo info(){{return {{"{name}"}};}} }}; }}\n')
            (package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
            manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp', 'type': f'compositions::{name}::Agent',
                        'sources': ['source/agent.cpp', '../../source/agent.cpp']}
            (package / 'agent.json').write_text(json.dumps(manifest, indent=2)+'\n')
            (package / 'IMPORT.json').write_text(json.dumps({**lineage, 'sample_count': count, 'objective': objective, 'status': 'Unvalidated experiment'}, indent=2)+'\n')
            (package / 'README.md').write_text(f'# {name}\n\nAtakan three-course selection at step226. Sampling mode{count}, objective {objective}. See ../../README.md and IMPORT.json for source lineage and validation scope.\n')
    print(f'Prepared {len(NAMES)} C++ packages.')


if __name__ == '__main__':
    main()
