from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
reference = json.loads((EXP / 'CURRENT_REFERENCE.json').read_text())
assert reference['name'] == 'observed_sale_lead_start_216'
parent = EXP / reference['path']
manifest = json.loads((parent / 'agent.json').read_text())
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for mode in [0, 1, 2]:
    name = f'empty_sale_slots_m{mode}'
    path = RUN / 'proposals' / name
    (path / 'source').mkdir(parents=True, exist_ok=False)
    (path / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../../observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public kag::agents::observed_sale_lead_start_216::Agent {{
    int removed_=0;
public:
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
    void reset(const kag::agent::AgentInit& init) {{kag::agents::observed_sale_lead_start_216::Agent::reset(init);removed_=0;}}
    int removed_slots()const{{return removed_;}}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {{
        kag::agents::observed_sale_lead_start_216::Agent::act(o,b,a);
        int kept=0;
        for(int i=0;i<a.n_orders;++i) {{
            const auto order=a.orders[i];
            bool eligible={mode}==1 || ({mode}==2 && o.step>=216 && order.item>kag::WHEAT && order.item<kag::FERTILIZER);
            if(eligible && order.op==kag::M_SELL && order.n<=0){{++removed_;continue;}}
            a.orders[kept++]=order;
        }}
        a.n_orders=kept;a.finalize();
    }}
}};
}}
''')
    (path / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    sources = ['source/agent.cpp']
    for source in manifest['sources']:
        target = (parent / source).resolve()
        sources.append(str(target.relative_to(ROOT)))
    # Manifest sources are relative to the agent folder, not the repo root.
    import os
    sources = [sources[0], *[os.path.relpath(ROOT / s, path) for s in sources[1:]]]
    (path / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
        'type': f'compositions::{name}::Agent', 'sources': sources}, indent=2) + '\n')
    (path / 'README.md').write_text(f'# {name}\n\nExact accepted parent with empty-SELL slot experiment. Mode0 control, mode1 removes all zero/negative SELL requests, mode2 removes only non-input empty sales from step216 onward. Positive order quantities, worker actions and all inherited branches remain requested as before; later market order positions can change realized economics. Motivation and lineage in ../../LINEAGE.json. Experimental, no promotion.\n')
    assert name not in catalog
    catalog[name] = str(path.relative_to(ROOT))
    names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
paths = [parent / 'source/agent.hpp', parent / 'agent.json', RUN / 'prepare.py']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(), 'parent': reference,
    'variants': names, 'evidence': 'runs/arlene_v4_sep08_001/CLAMP_WITNESSES.json',
    'hypothesis': 'Empty SELL slots are not economically neutral when a later order moves relative to the rival. Test removal on the accepted agent, with a separate late non-input-only version preserving its initial funding.',
    'lineage': 'New local output-order transform, motivated by exact Arlene source-clamp failure. No new external policy code copied; all inherited source lineage remains intact.',
    'discovery': {'seeds': 64, 'seed_start': 1000, 'seat_mode': 'both', 'opponents': ['observed_sale_lead_start_216', 'public_router_v52', 'ahmed_v23', 'junghoon_wool_sales', 'john_131', 'teammate_shoprouter', 'king_rc4', 'bohann_opening_v1']},
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}}, indent=2) + '\n')
script = (EXP / 'runs/arlene_v4_sep08_001/run_discovery.py').read_text()
script = script.replace("assert json.loads((RUN / 'SOURCE_PARITY.json').read_text())['result'] == 'matched_actions=8628'\n", '')
script = script.replace("+ ['public_capacity_router', 'public_terminal_router']", "+ ['observed_sale_lead_start_216']")
start = script.index('opponents = ')
end = script.index('\ncommand = ', start)
script = script[:start] + "opponents = json.loads((RUN / 'LINEAGE.json').read_text())['discovery']['opponents']" + script[end:]
script = script.replace('8960 profiled discovery games complete.', '4096 profiled discovery games complete.')
(RUN / 'run_discovery.py').write_text(script)
print('Prepared three incumbent empty-sale-slot comparisons.')
