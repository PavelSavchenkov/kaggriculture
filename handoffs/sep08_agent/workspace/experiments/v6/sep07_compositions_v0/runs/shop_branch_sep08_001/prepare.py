from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
fit = json.loads((RUN/'FIT_RESULT.json').read_text())
assert fit['selected_rule'] == [
    {'feature':1, 'operator':'eq', 'threshold':0},
    {'feature':2, 'operator':'eq', 'threshold':0},
]
prefix = json.loads((RUN/'PREFIX.json').read_text())
assert prefix['total'] == prefix['equal_through_step143'] == 1632
registry_path = EXP/'configs/league.json'
registry = json.loads(registry_path.read_text())
names = []
for mode in range(3):
    name = f'shop_branch_m{mode}'
    folder = RUN/'proposals'/name
    (folder/'source').mkdir(parents=True, exist_ok=False)
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../selector.hpp"
namespace compositions::{name} {{
class Agent: public shop_branch::Agent<{mode}> {{
public:
    static kag::agent::AgentInfo info() {{ return {{"{name}"}}; }}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    manifest = {'format_version':1, 'name':name, 'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent', 'sources':['source/agent.cpp',
        '../../../joint_day_routes_sep08_001/compiler/source/agent.cpp']}
    (folder/'agent.json').write_text(json.dumps(manifest, indent=2)+'\n')
    (folder/'README.md').write_text(f'''# {name}

Mode {mode}: 0 retains service_bank_p362_m2, 1 uses cold_renewal_p98,
2 chooses the cow-renewal farm on day6 when neither of the first two shops
consumes eggs or wool. Choice is latched from legal observations. Both child
states are advanced through the common opening and reset per episode.
The rule was fitted retrospectively; full-game parity and fresh validation
remain separate evidence. See ../../LINEAGE.json and ../../FIT_RESULT.json.
No external controller code newly copied; inherited farm/compiler origins
remain in the referenced experiment packages. Kaggle rating unknown.
''')
    assert name not in registry
    registry[name] = str(folder.relative_to(ROOT))
    names.append(name)
registry_path.write_text(json.dumps(registry, indent=2)+'\n')
report = {'created_utc':datetime.now(timezone.utc).isoformat(), 'variants':names,
    'baseline':'service_bank_p362_m2', 'alternative':'cold_renewal_p98',
    'ideas':['Choose an entire dated composition suffix from only observed shops.',
        'Use a verified common action prefix so standalone outcomes can represent an executable branch.',
        'Retain the faithful fitted rule before trying improved constituent farms.'],
    'fit':'FIT_RESULT.json', 'prefix':'PREFIX.json',
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()
        for p in [RUN/'selector.hpp', RUN/'prepare.py', RUN/'FIT_RESULT.json', RUN/'PREFIX.json']}}
(RUN/'LINEAGE.json').write_text(json.dumps(report, indent=2)+'\n')
print('Prepared', names, flush=True)
