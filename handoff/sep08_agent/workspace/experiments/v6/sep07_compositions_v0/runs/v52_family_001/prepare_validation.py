from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
name = 'wool_family_context_v1'
package = RUN / 'proposals' / name
assert not package.exists()
(package / 'source').mkdir(parents=True)
(package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/improved.hpp"\n'
    f'namespace kag::agents::{name}{{class Agent:public compositions::v52_family::OptimizedTransfer{{public:Agent():OptimizedTransfer(2,6){{}}'
    f'static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
(package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp', 'type': f'kag::agents::{name}::Agent',
            'sources': ['source/agent.cpp', *[os.path.relpath(EXP / f'league/{dep}/source/agent.cpp', package) for dep in ['top_replay_library', 'public_router']]]}
(package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
record = {'parent': 'late_value_s32_t0_r05', 'donor': 'public_router_v52', 'source_lineage': 'league/public_router_v52/IMPORT.json',
          'calendar_lineage': 'runs/v52_family_001/DAY_LIBRARY.json',
          'entry_rule': 'Atday6, YarnStore was first and the second shop is not PetCafe; or YarnStore was second and the first shop is PizzaShop or SmoothieShop.',
          'physical_and_funding_guard': 'Same source tile/worker obligations, observed cash>=700 and checked own non-input shed items.',
          'execution': 'Restore missing source stocks, follow the complete sheep-heavy suffix, and use17 source-day reductions behind exact guards.',
          'selection': 'Six simple observed-shop rules compared on6144 paired diagnosis contexts (12288 games), all1024 seeds reused1000..2023. Rule6 has positive means and nondecreasing utility for allthree sampled opponents. No diagnosis split is called untouched.',
          'limitations': ['A larger whole-farm source continuation, not unrestricted construction.', 'Estimated context value comes from sampled exact counterfactuals; no generic pre-routing evaluator is claimed.',
                          'No new official catalog copy or submission.', 'Original donor replay authorship and Kaggle rating unknown; public code borrowing user-authorized.'],
          'status': 'Frozen candidate awaiting executable-selector parity, fresh league and operational checks.'}
(package / 'IMPORT.json').write_text(json.dumps(record, indent=2) + '\n')
(package / 'README.md').write_text('# wool_family_context_v1\n\nIncumbent opening and complete V5/2 sheep-heavy continuation selected from the first two observed shops atday6. '
    'Restores source inputs and uses17 locally improved days when their physical guards match. '
    'Outside the entry condition it retains the incumbent. Full lineage and limits in IMPORT.json. Frozen experimental candidate, not promoted.\n')
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text());assert name not in catalog
catalog[name] = str(package.relative_to(ROOT));catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')

validation = EXP / 'runs/wool_family_validation_001'
validation.mkdir(exist_ok=False)
prior = json.loads((EXP / 'runs/late_portfolio_validation_001/FRESH_PREREGISTERED.json').read_text())
grouping = prior['grouping']
assert 'public_router_v52' not in [o for group in grouping.values() for o in group]
grouping['public_controllers'].append('public_router_v52')
opponents = sorted(set(prior['opponents'] + ['late_value_s32_t0_r05', 'public_router_v52']))
spec = {'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': name, 'baseline': 'late_value_s32_t0_r05',
        'seed_start': 1870000, 'seeds': 512, 'seat_mode': 'both', 'opponents': opponents,
        'grouping': grouping, 'historical_grouping': prior['historical_grouping'],
        'gates': prior['gates'], 'scope': record['selection'],
        'extra_requirement': 'NewpublicV5/2 is included in current grouping before results; exact source/course and runtime-selector parity required.',
        'candidate_files_sha256': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in [*(RUN / 'source').glob('*'), *package.rglob('*')] if p.is_file()}}
(validation / 'FRESH_PREREGISTERED.json').write_text(json.dumps(spec, indent=2) + '\n')
print(validation, 'opponents', len(opponents))
