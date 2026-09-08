"""Test starting sale lead after the initial farm-funding phase."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
OLD=EXP/'runs/observed_sale_lead_003'
(RUN/'source').mkdir(exist_ok=False)
source=(OLD/'source/policy.hpp').read_text()
source=source.replace('../parent_source/', '../../observed_sale_lead_003/parent_source/')
source=source.replace('observed_sale_lead_shared', 'observed_sale_lead_after_funding')
source=source.replace('int mode_=0,due_', 'int start_=24,mode_=0,due_')
source=source.replace('explicit Policy(int mode=1):mode_(mode){}', 'explicit Policy(int mode=1,int start=24):start_(start),mode_(mode){}')
source=source.replace('if(o.step<24 ||', 'if(o.step<start_ ||')
(RUN/'source/policy.hpp').write_text(source)
variants={f'observed_sale_lead_start_{start}':start for start in [216,240,288,360]}
catalog_path=EXP/'configs/league.json';catalog=json.loads(catalog_path.read_text())
for name,start in variants.items():
    p=RUN/'proposals'/name;(p/'source').mkdir(parents=True,exist_ok=False)
    (p/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
namespace kag::agents::{name} {{
class Agent:public compositions::observed_sale_lead_after_funding::Policy {{
public:
    Agent():Policy(1,{start}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (p/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    sources=['source/agent.cpp']+[os.path.relpath(OLD/f'parent_source/league/{n}/source/agent.cpp',p) for n in ['top_replay_library','public_router']]
    (p/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'kag::agents::{name}::Agent','sources':sources},indent=2)+'\n')
    (p/'README.md').write_text(f'# {name}\n\nAdvance noninput product sales starting at step{start}. Preserve the initial funding phase; use the same verified copied-policy forecast and immutable calendars as observed_sale_lead_shared_v3. Diagnosis: Jun1044 has one fewer successful sheep purchase at217 after earlier sales, despite unchanged rival actions, and that helps the rival later. Global start-time comparison, no opponent identity gate. Unpromoted. Full lineage in ../../LINEAGE.json.\n')
    assert name not in catalog;catalog[name]=str(p.relative_to(ROOT))
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'parent':'rival_wool_context_v3','sale_lead_source':'runs/observed_sale_lead_003/source/policy.hpp','source_sha256':hashlib.sha256((OLD/'source/policy.hpp').read_bytes()).hexdigest(),'upstream':'league/ahmed_v23/IMPORT.json','variants':variants,'diagnosis':'runs/observed_sale_lead_003/JUN_WITNESSES.json','hypothesis':'Preserve the initial farm financing phase. Earlier sales can prevent a rival investment and accidentally improve its later production; do not treat fixed rival actions as fixed rival outcomes. Compare global start times on the exposed ten-opponent panel before any fresh test.'},indent=2)+'\n')
print('Registered four start-time alternatives.')
