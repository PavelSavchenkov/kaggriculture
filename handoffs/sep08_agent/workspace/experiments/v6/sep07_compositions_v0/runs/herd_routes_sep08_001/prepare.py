from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
assert json.loads((EXP/'runs/compiler_split_sep08_001/PARITY.json').read_text())['full_games']==192
catalog_path=EXP/'configs/league.json';catalog=json.loads(catalog_path.read_text());names=[]
for case in ['mixed','goose','p355','p362']:
    for mode in [0,1]:
        name=f'herd_routes_{case}_m{mode}';folder=RUN/'proposals'/name;(folder/'source').mkdir(parents=True,exist_ok=False)
        (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "../../../routes.hpp"
#include "../../../../compiler_split_sep08_001/proposals/compiler_split_{case}/source/agent.hpp"
namespace compositions::{name} {{
class Agent:public herd_routes_sep08::Agent<compiler_split_{case}::Agent,{mode}> {{
public:
    static kag::agent::AgentInfo info() {{return {{"{name}"}};}}
}};
}}
''')
        (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (folder/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
            'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../../compiler_split_sep08_001/compiler/source/agent.cpp']},indent=2)+'\n')
        (folder/'README.md').write_text(f'# {name}\n\nCold {case} farm, routes mode{mode}. From day25, once no live crops remain, assign observed animals among current workers and carry the feed required by each route. Mode0 preserves the split compiler. Hiring, farm composition and market policy are unchanged; market projection uses actual route actions. Current observation only; no source schedule/seed/future shops. Scope and lineage in ../../LINEAGE.json. Experimental, not promoted.\n')
        assert name not in catalog;catalog[name]=str(folder.relative_to(ROOT));names.append(name)
catalog_path.write_text(json.dumps(catalog,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'variants':names,
    'discovery':{'seed_start':1000,'seeds':8,'seat_mode':'both','opponents':['public_router','observed_sale_lead_start_216']},
    'parent':'runs/compiler_split_sep08_001','evidence':['runs/dated_day_service_sep08_002/RESULTS.json','runs/compiler_feed_bundle_sep08_001/ANALYSIS.json'],
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [RUN/'routes.hpp',RUN/'prepare.py',EXP/'runs/compiler_split_sep08_001/compiler/source/agent.cpp']},
    'ideas':['Assign animal visits and wheat jointly from current positions/inventories, keeping route ownership through the day.',
             'Plan each worker complete remaining feed withdrawal and return surplus wheat to other routes.',
             'Use actual physical actions in the inherited market projection; retain old controls and observation restrictions.'],
    'limits':['First reusable routing component handles the crop-free end of these four cold farms only; it does not yet solve mixed crop/animal days.',
              'Greedy insertion is a route heuristic; exact day-solver schedules establish feasibility but do not certify this heuristic.',
              'Productive service is inherited for these local cold families. No universality claim for source strategies that intentionally skip service.']},indent=2)+'\n')
(RUN/'run_discovery.py').write_bytes((EXP/'runs/compiler_split_sep08_001/run_discovery.py').read_bytes())
print('Prepared eight crop-free herd routing policies.')
