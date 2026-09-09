"""Port the reviewed four-tape router; retain the original and local ablations."""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import sys

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
sys.path.insert(0, str(EXP / 'scripts'))
from generate_public_routes import triple

upstream = EXP / 'research/refresh_sep08_1108/notebook_audit/yusuke'
tapes = json.loads((upstream / 'actions.json.txt').read_text())
model = json.loads((upstream / 'model.json.txt').read_text())
assert model == {'kind':'mmpq_multi_entry_v1','default':0,'stages':[
    {'step':144,'tree':{'feature':23,'threshold':0.5,'left':{'choice':0},'right':{'choice':1}}},
    {'step':648,'tree':{'feature':71,'threshold':9888.0,'left':{'choice':2},'right':{'choice':3}}}]}
assert len(tapes) == 4 and all(len(t) == 719 for t in tapes)
offsets, values = [], []
for tape in tapes:
    offsets.append([])
    for action in tape:
        offsets[-1].append(len(values))
        units = [triple(u or ['PASS']) for u in [action['farmer'], *action['hands']]]
        orders = [triple(o or ['PASS'], True) for o in action['market']]
        assert len(orders) <= 10
        values += [len(units), len(orders), *(n for v in units + orders for n in v)]
code = '// Literal upstream actions; see LINEAGE.json.\nconstexpr int offsets[4][719]={\n'
code += ',\n'.join('{' + ','.join(map(str, row)) + '}' for row in offsets) + '\n};\nconstexpr int values[]={\n'
code += ',\n'.join(','.join(map(str, values[i:i+100])) for i in range(0,len(values),100)) + '\n};\n'
(RUN / 'source').mkdir(exist_ok=True)
(RUN / 'source/data.inc').write_text(code)
registry_path = EXP / 'configs/league.json'
registry = json.loads(registry_path.read_text())
for mode in range(4):
    name = f'yusuke_sep08_m{mode}'
    folder = RUN / 'proposals' / name
    (folder / 'source').mkdir(parents=True, exist_ok=False)
    (folder / 'source/agent.hpp').write_text(f'''#pragma once
#include "../../../source/policy.hpp"
namespace compositions::{name} {{
class Agent:public yusuke_port::Policy {{
public:
    Agent():Policy({mode}){{}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (folder / 'source/agent.cpp').write_text('#include "agent.hpp"\n')
    (folder / 'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent','sources':['source/agent.cpp','../../source/policy.cpp']},indent=2)+'\n')
    (folder / 'README.md').write_text(f'''# {name}

Experimental C++ port of Yusuke Hayashi's Shop Router0908 public notebook.
Mode{mode}:0base tape;1day6Yarn choice;2full upstream day6/day27router;
3local amendment preserves the day6alternative instead of switching it into
a terminal continuation constructed from the other farm. This amendment needs
causal validation and is not silently part of the faithful source port.

All four719-action tapes are immutable; route state is per instance/reset.
Only observed Yarn shops and public egg inventory determine the two decisions.
Original source/metadata/model/tape hashes are in ../../LINEAGE.json. Donor team
rank84/rating2663.1 at11:10UTC does not verify this artifact's rating. Original
replay provenance and independent source license are unspecified. Retain
attribution. Source parity, operational checks and playing strength pending.
''')
    assert name not in registry
    registry[name] = str(folder.relative_to(ROOT))
registry_path.write_text(json.dumps(registry,indent=2)+'\n')
prefix = [[next((i for i in range(719) if a[i]!=b[i]),719) for b in tapes] for a in tapes]
(RUN / 'LINEAGE.json').write_text(json.dumps({'created_utc':datetime.now(timezone.utc).isoformat(),
    'source':'https://www.kaggle.com/code/yhay81/shop-router-0908',
    'sources':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in upstream.iterdir() if p.is_file()},
    'prefix_matrix':prefix,'model':model,
    'feature_mapping':{'23':'count of already observed Yarn Store shops','71':'public egg inventory'},
    'ideas':['Use a whole alternate farm when Yarn has been observed by day6.',
        'Choose a final three-day continuation from current egg stock at day27.',
        'Local hypothesis: preserve branch1 because terminal tapes2/3 share their prefix with branch0.'],
    'scope':'Literal frozen model and four tapes; no hidden observation, seed or future shop access.',
    'provenance_limits':'Original replay episodes and artifact rating unknown. No explicit source reuse license in downloaded metadata.'},indent=2)+'\n')
print('Prepared four Yusuke variants; mode2 is faithful upstream.')
