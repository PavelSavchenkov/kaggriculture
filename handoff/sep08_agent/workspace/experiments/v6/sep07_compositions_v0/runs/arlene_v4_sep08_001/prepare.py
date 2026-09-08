from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
old = EXP / 'league/public_capacity_router/source'
folder = RUN / 'source'
folder.mkdir(exist_ok=True)
code = (old / 'agent.cpp').read_text().replace('namespace compositions::public_capacity_router', 'namespace compositions::arlene_v4_sep08')
code = code.replace('#include <algorithm>', '#include <algorithm>\n#include <cmath>')
code = code.replace('../../public_router/source/routes.inc', '../../../league/public_router/source/routes.inc')
code = code.replace('std::min(tape_units, action.n_units); ++u)', '((mask_&16)?action.n_units:std::min(tape_units, action.n_units)); ++u)')
code = code.replace('int room = 100 - obs.own.shed_total;', 'int room = (mask_&8)?std::max(0,100-obs.own.shed_total):100-obs.own.shed_total;')
code = code.replace('std::min(tape_units, action.n_units) && room > 0;', 'action.n_units && ((mask_&8) || room > 0);')
code = code.replace('        if (a.op == OP_DROP) {', '''        if((mask_&8) && a.op==OP_PICKUP) {
            int take=std::min(std::max(0,projected[a.arg]),std::max(1,a.n));
            projected[a.arg]-=take;room+=take;
        } else if (a.op == OP_DROP) {''')
code = code.replace('std::min({a.n, int(obs.own.inv[u][a.arg]), room})', 'std::min({(mask_&8)?std::max(1,a.n):a.n, int(obs.own.inv[u][a.arg]), room})')
code = code.replace('std::min(tape_units,action.n_units);++u)', '((mask_&16)?action.n_units:std::min(tape_units,action.n_units));++u)')
anchor = '// The donor calls this only on weeds.'
assert code.count(anchor) == 1
code = code.replace(anchor, '#include "../budget.hpp"\n\n' + anchor)
anchor = '    if(step%24==23) {'
assert code.count(anchor) == 1
code = code.replace(anchor, '    if(mask_&1)budget_guard(obs,data,route_,action);\n\n' + anchor)
code = code.replace('            order.n = std::min(order.n, available[order.item]);\n            if (order.n <= 0) continue;',
    '            order.n = (mask_&2)?std::min(std::max(0,order.n),std::max(0,available[order.item])):std::min(order.n,available[order.item]);\n            if (!(mask_&2) && order.n <= 0) continue;')
code = code.replace('        action.orders[kept++] = order;\n    }\n    action.n_orders = kept;',
    '        if((mask_&2) && order.op==M_BUY_PRODUCT)available[order.item]+=std::max(0,order.n);\n        action.orders[kept++] = order;\n    }\n    action.n_orders = kept;')
code = code.replace('if(terminal_ && step==718)', 'if((mask_&4) && step==718)')
(folder / 'agent.cpp').write_text(code)
header = (old / 'agent.hpp').read_text().replace('compositions::public_capacity_router', 'compositions::arlene_v4_sep08')
header = header.replace('bool terminal_;', 'int mask_;').replace('explicit AgentCore(bool terminal=false):terminal_(terminal)', 'explicit AgentCore(int mask=31):mask_(mask)')
(folder / 'agent.hpp').write_text(header)
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for mask in [0, 1, 2, 4, 7, 8, 16, 31]:
    name = f'arlene_v4_m{mask}'
    path = RUN / 'proposals' / name
    (path / 'source').mkdir(parents=True, exist_ok=True)
    (path / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/agent.hpp"
namespace compositions::{name} {{
class Agent:public arlene_v4_sep08::AgentCore {{
public:
    Agent():AgentCore({mask}) {{}}
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
    (path / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (path / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
        'type': f'compositions::{name}::Agent', 'sources': ['source/agent.cpp', '../../source/agent.cpp']}, indent=2) + '\n')
    (path / 'README.md').write_text(f'# {name}\n\nArlene V4 public notebook combination on the known Thomas four-route controller. Mask {mask}: bit1 budget, bit2 sequential clamp, bit4 terminal, bit8 pickup-aware projection, bit16 padded-worker weed repair. Mask31 is the complete source port; others are local causal ablations. Rating unknown, source parity and operational validation pending. Exact source and reuse limits in ../../LINEAGE.json. Observation-only, per-instance route selection.\n')
    assert name not in catalog or catalog[name] == str(path.relative_to(ROOT))
    catalog[name] = str(path.relative_to(ROOT))
    names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
reference = EXP / 'research/refresh_sep08_0308/notebook_audit/arlene/decoded_main.py'
paths = [reference, old / 'agent.cpp', old / 'agent.hpp', EXP / 'league/public_router/source/routes.inc', RUN / 'budget.hpp', RUN / 'prepare.py', folder / 'agent.cpp', folder / 'agent.hpp']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'source': 'https://www.kaggle.com/code/lynnsakurai/farming-score-v4-a-better-shop',
    'reference': str(reference.relative_to(EXP)), 'reference_sha256': hashlib.sha256(reference.read_bytes()).hexdigest(),
    'donor': 'Known Thomas four-route tape and tetsutani capacity controller; seven functions and compressed route blob match exactly.',
    'local_changes': 'Typed C++ port from the already verified capacity-router C++; literal new budget, sequential clamp, final settlement, pickup-aware projection and padded-worker weed repair. Isolated bit masks are local ablations.',
    'rating': 'Unknown; notebook performance claims are not locally validated.',
    'reuse_restrictions': 'User authorized public code borrowing. No separate license stated in retrieved notebook metadata; retain source attribution and do not infer a license.',
    'variants': names, 'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}, indent=2) + '\n')
print('Prepared eight Arlene V4 policies; source parity next.')
