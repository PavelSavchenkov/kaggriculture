from pathlib import Path
from datetime import datetime,timezone
import json
import os

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
registry_path=EXP/'configs/league.json';registry=json.loads(registry_path.read_text())
parent=ROOT/registry['empty_sale_slots_m2']
parent_sources=[(parent/p).resolve() for p in json.loads((parent/'agent.json').read_text())['sources']]
for quantity in (32,13,20,24):
    name=f'opening_funding_q{quantity}'
    folder=RUN/'proposals'/name;(folder/'source').mkdir(parents=True,exist_ok=False)
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
namespace compositions::{name} {{
class Agent:public opening_funding::Policy {{
public:
    Agent():Policy({quantity}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent','sources':['source/agent.cpp',*[os.path.relpath(p,folder) for p in parent_sources]]},indent=2)+'\n')
    (folder/'README.md').write_text(f'''# {name}

Experimental strongest-parent opening round trip of{quantity}wheat instead of32.
Only the two quantities at step0change; order slots, initial farm wheat,
step1funding, herd/crop branches and market/service controllers are inherited.
Quantity32is an exact parent control. Quantity13comes from the new Yusuke public
router;20/24are local intermediate hypotheses. See ../../LINEAGE.json.

An exposed seed1000causal intervention at step0changed own finalcash+552and
rivalcash-9512, restoring later execution. Copying both donor market turns
instead broke our existing worker plan and made two cows escape. Do not infer
broad strength from this witness. Required operations and league tests pending.
''')
    assert name not in registry;registry[name]=str(folder.relative_to(ROOT))
registry_path.write_text(json.dumps(registry,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'parent':'runs/empty_sale_slots_sep08_001/proposals/empty_sale_slots_m2',
    'idea_source':'https://www.kaggle.com/code/yhay81/shop-router-0908',
    'witness':'runs/animal_group_policy_sep08_001/GAP_WITNESS.txt',
    'changes':'Only first-turn wheat buy/sell quantities32->13/20/24. Entire accepted policy remains underneath.',
    'limits':'Initial quantity cannot observe the unseen opponent. Direct counters may regress other opponents; test Bohann and old opening versions as well as the current league.',
    'negative':'Copying both market turns adds an incompatible worker and changes funding; two cow escapes in witness. Rejected.'},indent=2)+'\n')
print('Prepared opening-funding variants and exact q32control')
