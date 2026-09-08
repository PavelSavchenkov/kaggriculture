from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
parent = EXP / 'runs/observed_sale_lead_004/proposals/observed_sale_lead_start_216'
manifest = json.loads((parent / 'agent.json').read_text())
catalog_path = EXP / 'configs/league.json'
catalog = json.loads(catalog_path.read_text())
names = []
for mode in [0, 1, 2, 3]:
    name = f'empty_sale_floor_m{mode}'
    path = RUN / 'proposals' / name
    (path / 'source').mkdir(parents=True, exist_ok=False)
    (path / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../../observed_sale_lead_004/proposals/observed_sale_lead_start_216/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public kag::agents::observed_sale_lead_start_216::Agent {{
    int removed_=0,protected_=0;
public:
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
    void reset(const kag::agent::AgentInit& init) {{
        kag::agents::observed_sale_lead_start_216::Agent::reset(init);removed_=protected_=0;
    }}
    int removed_slots()const{{return removed_;}}
    int protected_slots()const{{return protected_;}}
    void act(const kag::agent::AgentObservation& o,const kag::agent::DecisionBudget& b,kag::Action& a) {{
        kag::agents::observed_sale_lead_start_216::Agent::act(o,b,a);
        if(o.step<216)return;
        bool unsafe[10]{{}};
        if constexpr({mode}>0) {{
            std::array<int,kag::N_PRODUCTS> inventory;
            std::copy_n(o.market.inventory,kag::N_PRODUCTS,inventory.begin());
            for(int i=0;i<a.n_orders;++i) {{
                const auto& order=a.orders[i];
                if(order.op==kag::M_BUY_PRODUCT && order.n>0)inventory[order.item]-=order.n;
                if(order.op!=kag::M_SELL || order.n<=0)continue;
                inventory[order.item]+=order.n;
                const int rival_reserve={mode}==2?int(order.n):({mode}==3?100:0);
                unsafe[i]=kag::market_price(order.item,inventory[order.item]+rival_reserve)<=1;
            }}
        }}
        int kept=0;
        for(int i=0;i<a.n_orders;++i) {{
            const auto order=a.orders[i];
            const bool eligible=order.op==kag::M_SELL && order.n<=0 && order.item>kag::WHEAT && order.item<kag::FERTILIZER;
            bool protected_slot=false;
            if(eligible)for(int j=i+1;j<a.n_orders;++j)protected_slot|=unsafe[j];
            if(eligible && !protected_slot){{++removed_;continue;}}
            protected_+=protected_slot;
            a.orders[kept++]=order;
        }}
        a.n_orders=kept;a.finalize();
    }}
}};
}}
''')
    (path / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    sources = ['source/agent.cpp', *[os.path.relpath((parent / source).resolve(), path) for source in manifest['sources']]]
    (path / 'agent.json').write_text(json.dumps({'format_version': 1, 'name': name, 'header': 'source/agent.hpp',
        'type': f'compositions::{name}::Agent', 'sources': sources}, indent=2) + '\n')
    (path / 'README.md').write_text(f'# {name}\n\nLate empty non-input sale removal with price-floor guard, mode{mode}. Mode0 reproduces empty_sale_slots_m2. Mode1 protects later sales whose own requested volume reaches price1; mode2 adds matching rival sale volume; mode3 adds100 units for the rival shed capacity. Counts are observations and own requested trades only. Guards are heuristics: actual fills, rival purchases and rival supply are not known. All inherited behavior and source lineage retained. Experimental, not promoted. See ../../LINEAGE.json.\n')
    assert name not in catalog
    catalog[name] = str(path.relative_to(ROOT))
    names.append(name)
catalog_path.write_text(json.dumps(catalog, indent=2) + '\n')
old = EXP / 'runs/empty_sale_slots_sep08_001'
spec = json.loads((old / 'LINEAGE.json').read_text())
source_paths = [RUN / 'prepare.py', parent / 'source/agent.hpp', parent / 'agent.json']
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc': datetime.now(timezone.utc).isoformat(),
    'parent': 'observed_sale_lead_start_216', 'control': 'empty_sale_slots_m2', 'variants': names,
    'evidence': 'runs/empty_sale_validation_sep08_001/TRACE_ANALYSIS.json',
    'hypothesis': 'A compressed strawberry sale at598 changes floor-price market inventory by one unit and eventually loses6 margin despite no adverse immediate effect. Preserve slots when shifted sales can reach the floor. Compare own-volume, matched-volume and shed-capacity reserves.',
    'lineage': 'New local market-output guard. Motivated by exact full-game native audit failure and official C++ engine floor semantics; no external policy code newly copied. All inherited external replay/notebook lineage remains.',
    'discovery': spec['discovery'],
    'audit_separation': 'All discovery seeds are previously exposed. The failed native seed2004097 is causal diagnosis only. No new fresh audit is claimed or launched; any selected candidate requires an unused preregistered pool. The old frozen candidate remains rejected and unchanged.',
    'source_hashes': {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in source_paths}}, indent=2) + '\n')
script = (old / 'run_discovery.py').read_text().replace("+ ['observed_sale_lead_start_216']", '')
(RUN / 'run_discovery.py').write_text(script)
(RUN / 'README.md').write_text('# Empty-sale price-floor guards\n\nRead LINEAGE.json for the exact failed native witness and economic hypothesis. prepare.py creates four C++ policies; mode0 must reproduce the existing empty-sale candidate. run_discovery.py runs4096 previously exposed full games across eight opponents. No fresh validation or promotion yet.\n')
print('Prepared four empty-sale price-floor comparisons; discovery only.')
