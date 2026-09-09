"""Wrap the current reference with the measured public sale-lead idea."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
variants={'sale_lead_fast_control':0,'observed_sale_lead_fast_v2':1,'observed_sale_lead_fast_milk_wool':2}
catalog_path=EXP/'configs/league.json';catalog=json.loads(catalog_path.read_text())
for name,mode in variants.items():
    p=RUN/'proposals'/name;(p/'source').mkdir(parents=True,exist_ok=False)
    (p/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::{name} {{
class Agent:public compositions::observed_sale_lead_fast::Policy {{
public:
    Agent():Policy({mode}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (p/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (p/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'kag::agents::{name}::Agent','sources':['source/agent.cpp','../../../../league/top_replay_library/source/agent.cpp','../../../../league/public_router/source/agent.cpp']},indent=2)+'\n')
    (p/'README.md').write_text(f'# {name}\n\nCurrent rival_wool_context_v3 with mode{mode}:0predict-only,1advance all non-input product sales,2milk/wool only. Local copied-policy next-turn forecast; AhmedV23 sale-lead, suppression and projected-shed ideas. Forecast skips day/shop boundaries and consumption turns; never reads actual future state. Exact lineage in ../../LINEAGE.json. Unpromoted discovery candidate.\n')
    assert name not in catalog;catalog[name]=str(p.relative_to(ROOT))
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
report={'created_utc':datetime.now(timezone.utc).isoformat(),'parent':'rival_wool_context_v3','variants':variants,
        'idea_source':'league/ahmed_v23/IMPORT.json; exact local component ablation runs/ahmed_components_001/ANALYSIS.json',
        'local_change':'Predict next raw market sales by copying the current policy and advancing only observed time. Verify actual next-turn requested sales; keep forecast error explicit.',
        'copied_logic':['Sequential projected shed','No-current-sale guard and non-input product filter','One-turn sale suppression, retaining zero-quantity slots'],
        'policy_sha256':hashlib.sha256((RUN/'source/policy.hpp').read_bytes()).hexdigest(),
        'limits':['Next observation is approximated by current public/own state with time advanced.','Prediction changes can affect inherited inventory guards and market behavior.','Copying policy increases CPU work; exact action checks and timing comparisons required.']}
(RUN/'LINEAGE.json').write_text(json.dumps(report,indent=2)+'\n')
print('Registered3 sale-lead variants, including an exact parent action control.')
