from pathlib import Path
from datetime import datetime,timezone
import hashlib,json
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
source=EXP/'research/refresh_sep08_0708/notebook_audit/farm_signal'
audit=json.loads((source/'SOURCE_AUDIT.json').read_text());assert audit['schedules_equal'] and audit['policy_tree_equal']
schedules,trees=json.loads((source/'DATA.json').read_text());assert len(schedules)==5 and len(trees)==10
products=['WHEAT','CARROT','TOMATO','STRAWBERRY','MELON','EGG','MILK','WOOL','FERTILIZER']
future=[]
for route in schedules:
 assert len(route)==719
 masks=[0]*720
 for step in range(718,-1,-1):
  masks[step]=masks[step+1]
  for o in route[step].get('market',[]):
   if o and o[0]=='SELL' and len(o)>2 and o[1] in products and int(o[2])>0:masks[step]|=1<<products.index(o[1])
 future.append(masks)
code='#pragma once\n#include <cstdint>\nnamespace compositions::farm_signal {inline constexpr uint16_t has_future[5][720]={\n'
code+=',\n'.join('{'+','.join(map(str,m))+'}' for m in future)+'};}\n'
(RUN/'future.hpp').write_text(code)
variants=[];catalog=json.loads((EXP/'configs/league.json').read_text())
for mode in [0,1]:
 name=f'farm_signal_v52_m{mode}';variants.append(name);p=RUN/'proposals'/name;(p/'source').mkdir(parents=True)
 (p/'source/agent.hpp').write_text(f'#pragma once\n#include "../../../policy.hpp"\nnamespace compositions::{name} {{class Agent:public farm_signal::Agent<{str(bool(mode)).lower()}> {{public:static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n')
 (p/'source/agent.cpp').write_text('#include "agent.hpp"\n')
 (p/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../../../league/public_router_v52/source/agent.cpp']},indent=2)+'\n')
 (p/'README.md').write_text(f'# {name}\n\nFaithful Farm Signal Engine capacity rule on verified publicV5/2, enabled={mode}. Control0 is unchanged V5/2. No new route or routing-tree logic. Per-instance state; only legal observations, no seed/private rival inventory. Source/parity scope in ../../LINEAGE.json. Current public rating and whole-controller strength unverified; no explicit separate license in pulled metadata. User authorizes reuse.\n')
 assert name not in catalog;catalog[name]=str(p.relative_to(ROOT))
(EXP/'configs/league.json').write_text(json.dumps(catalog,indent=2)+'\n')
(RUN/'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),'variants':variants,'source':'https://www.kaggle.com/code/flexonafft/kaggriculture-farm-signal-engine','source_path':str((source/'agent_source.py').relative_to(EXP)),'source_sha256':hashlib.sha256((source/'agent_source.py').read_bytes()).hexdigest(),'data_equal_to':'league/public_router_v52','copied_component':'_room_guard and _future_sells suffix-presence decisions','changes':['Typed C++ room guard; reuse already verified V5/2 controller.','Immutable suffix bitmasks replace repeated counting; only positivity is used.','Independent instance state and active-worker normalization follow local API.'],'limits':['Original five schedule episode provenance remains unavailable.','Source counts requested harvest/feed/placement effects even if unit actions fail; this approximation is intentionally preserved.','Pure guard parity plus unchanged base does not by itself prove competitive strength.'],'reuse':'Public notebook; no explicit separate license supplied. User explicitly authorizes component reuse.'},indent=2)+'\n')
print('Prepared two C++ V5/2 capacity variants.')
