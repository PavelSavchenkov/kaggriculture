from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
SOURCE = EXP / 'runs/v52_family_001'
source = RUN / 'source'
source.mkdir(exist_ok=False)
hashes = {}
for filename in ['entry.hpp', 'transfer.hpp', 'improved.hpp']:
    original = SOURCE / 'source' / filename
    hashes[str(original.relative_to(EXP))] = hashlib.sha256(original.read_bytes()).hexdigest()
    code = original.read_text().replace('namespace compositions::v52_family', 'namespace compositions::v52_family_v2')
    if filename == 'transfer.hpp':
        old = '(o.shops[0]==kag::SHOP_YARN_STORE && o.shops[1]!=kag::SHOP_PET_CAFE)'
        new = '''(o.shops[0]==kag::SHOP_YARN_STORE && (o.shops[1]==kag::SHOP_ICE_CREAM_SHOP ||
                 o.shops[1]==kag::SHOP_PIZZA_SHOP || o.shops[1]==kag::SHOP_SMOOTHIE_SHOP || o.shops[1]==kag::SHOP_YARN_STORE))'''
        assert old in code
        code = code.replace(old, new)
    (source / filename).write_text(code)

name = 'wool_family_context_v2'
package = RUN / 'proposals' / name
(package / 'source').mkdir(parents=True)
(package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/improved.hpp"\n'
    f'namespace kag::agents::{name}{{class Agent:public compositions::v52_family_v2::OptimizedTransfer{{public:Agent():OptimizedTransfer(2,6){{}}'
    f'static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
(package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
manifest = {'format_version': 1, 'name': name, 'header': 'source/agent.hpp', 'type': f'kag::agents::{name}::Agent',
            'sources': ['source/agent.cpp', *[os.path.relpath(EXP / f'league/{dep}/source/agent.cpp', package) for dep in ['top_replay_library', 'public_router']]]}
(package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
lineage = {'parent': 'late_value_s32_t0_r05', 'previous_candidate': 'wool_family_context_v1', 'donor': 'public_router_v52',
           'source_lineage': 'league/public_router_v52/IMPORT.json', 'calendar_lineage': 'runs/v52_family_001/DAY_LIBRARY.json',
           'upstream_files_sha256': hashes,
           'entry_rule': 'Atday6: firstYarn followed by IceCream/Pizza/Smoothie/Yarn, or secondYarn after Pizza/Smoothie.',
           'selection': 'Three revisions investigated after adding4096 Junghoon counterfactual games. Milk-or-double-Yarn retains the larger useful set. All16384 diagnosis games use1000..2023;1870000 validation fromv1 is now also exposed.',
           'changes': ['Independent namespace/source snapshot preservesv1 untouched', 'Restrict earlyYarn continuation to a milk shop or anotherYarn', 'All physical, stock restoration and17calendar execution details unchanged'],
           'limitations': ['Empirical context valuation from complete courses; arbitrary pre-routing construction remains incomplete.',
                           'The26 captured entryfeatures do not distinguish V5/2 from Junghoon, so this rule is shared across rival continuations.',
                           'Original donor episode provenance and rating unknown; public-code borrowing user-authorized.'],
           'status': 'Frozen experimental candidate, awaiting parity and new fresh validation.'}
(package / 'IMPORT.json').write_text(json.dumps(lineage, indent=2) + '\n')
(package / 'README.md').write_text('# wool_family_context_v2\n\nIncumbent opening with a complete sheep-heavy donor continuation fromday6, selected from the firsttwo observed shops. '
    'Uses the same checked stock restoration and17 day plans asv1. Independent namespace and source snapshot preservev1. '
    'Full lineage and exact rule in IMPORT.json. Experimental, not promoted.\n')
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text());assert name not in catalog
catalog[name] = str(package.relative_to(ROOT));catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')

validation = EXP / 'runs/wool_family_validation_002'
validation.mkdir(exist_ok=False)
prior = json.loads((EXP / 'runs/wool_family_validation_001/FRESH_PREREGISTERED.json').read_text())
spec = {**prior, 'created_utc': datetime.now(timezone.utc).isoformat(), 'candidate': name,
        'seed_start': 1890000, 'seeds': 512, 'opponents': sorted(set(prior['opponents'] + ['wool_family_context_v1'])),
        'scope': lineage['selection'], 'extra_requirement': 'v1 retained as an additional independent-family opponent; grouping and all numeric gates unchanged.',
        'candidate_files_sha256': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest()
                                  for p in [*source.glob('*'), *package.rglob('*')] if p.is_file()}}
(validation / 'FRESH_PREREGISTERED.json').write_text(json.dumps(spec, indent=2) + '\n')
for filename in ['run_fresh.py', 'analyze_fresh.py', 'run_checks.py', 'check_selector.py']:
    code = (EXP / 'runs/wool_family_validation_001' / filename).read_text()
    code = code.replace('wool_family_context_v1', 'wool_family_context_v2').replace('1873000', '1893000').replace('18701907', '18901907')
    if filename == 'check_selector.py':
        code = code.replace("'public_router']", "'public_router', 'junghoon_wool_sales']")
        code = code.replace("    alternatives = {a: json.loads((diagnosis /", "    folder = EXP / 'runs/v52_family_001/junghoon_counterfactuals' if opponent == 'junghoon_wool_sales' else diagnosis\n    alternatives = {a: json.loads((folder /")
        code = code.replace("features = list(csv.DictReader((diagnosis /", "features = list(csv.DictReader((folder /")
        code = code.replace('first == 7 and second != 4', 'first == 7 and second in [3, 5, 6, 7]')
    (validation / filename).write_text(code)
(RUN / 'LINEAGE.json').write_text(json.dumps(lineage, indent=2) + '\n')
(RUN / 'README.md').write_text('# Revised whole-farm wool context\n\nEntry point: LINEAGE.json and ../wool_family_validation_002. '
    'Source snapshot and namespace preserve the prior candidate. The actual additional branch remains a complete dated composition with its own service and trade calendar, not one extra animal.\n')
print('Prepared', name, 'with', len(spec['opponents']), 'fresh opponents.')
