"""Package the diagnosed missed sheep purchase repair for exact testing."""
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
NAME = 'rival_wool_purchase_repair_v1'
package = RUN / 'proposals' / NAME
(package / 'source').mkdir(parents=True, exist_ok=False)
manifest = {'format_version': 1, 'name': NAME, 'header': 'source/agent.hpp',
            'type': f'kag::agents::{NAME}::Agent', 'sources': ['source/agent.cpp',
            '../../../../league/top_replay_library/source/agent.cpp', '../../../../league/public_router/source/agent.cpp']}
(package / 'agent.json').write_text(json.dumps(manifest, indent=2) + '\n')
(package / 'source/agent.hpp').write_text('#pragma once\n#include "../../../source/repair.hpp"\n'
    f'namespace kag::agents::{NAME}{{using Agent=compositions::rival_wool_repair::Policy;}}\n')
(package / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
(package / 'README.md').write_text('''# Rival wool purchase repair v1

Experimental, not promoted. Preserve rival_wool_context_v3. If its additional
wool continuation buys only one of two requested sheep at step217, retry the
missing purchase when funds and shed space permit, through step252. Its recorded
pickup is at253. Never retry across a day-end transition or after that deadline.
No new worker schedule, placement or service rule is introduced.

See IMPORT.json for the source course and diagnostic evidence. Actual funding,
storage, sales and future guard matches require full-game validation.
''')
origin = {'parent': 'rival_wool_context_v3', 'donor': 'public_router_v52',
          'donor_provenance': 'league/public_router_v52/IMPORT.json',
          'calendar_lineage': 'runs/v52_family_001/DAY_LIBRARY.json',
          'diagnosis': 'runs/rival_wool_execution_001/EXECUTION_DIAGNOSIS.json',
          'idea': 'Honor an existing animal purchase obligation when a source order was partially funded, before its later pickup deadline.',
          'scope': 'Only the new delayed wool branch; retained parent choices remain unchanged.',
          'parity': '256 instrumented parent games match full native records; this repair needs new exact games.',
          'kaggle_rating': None, 'optimization_status': 'Unvalidated experiment',
          'reuse': 'User-authorized public source borrowing; donor details retained in its import.',
          'policy_sha256': hashlib.sha256((RUN / 'source/repair.hpp').read_bytes()).hexdigest()}
(package / 'IMPORT.json').write_text(json.dumps(origin, indent=2) + '\n')
registry = json.loads((EXP / 'configs/league.json').read_text())
assert NAME not in registry
registry[NAME] = str(package.relative_to(ROOT))
(EXP / 'configs/league.json').write_text(json.dumps(registry, indent=2) + '\n')
print('Prepared', NAME)
