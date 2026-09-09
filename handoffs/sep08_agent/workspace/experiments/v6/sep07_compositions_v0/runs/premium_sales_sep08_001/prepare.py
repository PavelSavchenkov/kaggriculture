"""Port AhmedV24's sole AST change and test it on the current strongest farm."""
from pathlib import Path
import argparse
import json
import os
import shutil

parser=argparse.ArgumentParser()
parser.add_argument('--finish',action='store_true')
args=parser.parse_args()
RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
donor=EXP/'research/refresh_sep08_1208/notebook_audit/ahmedberatozer__notebookd5e3d21fa6'
diff=json.loads((donor/'STATIC_DIFF.json').read_text())
assert diff['nodes']==[37,37] and len(diff['changed'])==1 and diff['changed'][0]['name']=='agent'
registry_path=EXP/'configs/league.json';registry=json.loads(registry_path.read_text())
specs=[('premium_sales_m0','empty_sale_slots_m2',720),('premium_sales_s144','empty_sale_slots_m2',144),
    ('premium_sales_s216','empty_sale_slots_m2',216),('ahmed_v24','ahmed_v23',144)]
for name,parent,first in ([] if args.finish else specs):
    p=RUN/'proposals'/name;(p/'source').mkdir(parents=True,exist_ok=False)
    parent_path=ROOT/registry[parent];manifest=json.loads((parent_path/'agent.json').read_text())
    header=(parent_path/manifest['header']).relative_to(ROOT)
    (p/'source/agent.hpp').write_text(f'''#pragma once
#include "{header}"
#include "../../../source/policy.hpp"
namespace compositions::{name} {{
class Agent:public premium_sales::Policy<{manifest['type']},{first}> {{
public:
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (p/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    sources=[os.path.relpath((parent_path/s).resolve(),p) for s in manifest['sources']]
    (p/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent','sources':['source/agent.cpp',*sources]},indent=2)+'\n')
    (p/'README.md').write_text(f'''# {name}

Parent {parent}; stable positive non-input SELL priority from step{first}.
Copied behavior: AhmedV24's entry-point market-order transform, which credits
KingRC4's transaction-ordering idea. Source notebook and exact AST difference
are in ../../IMPORT.json. Relative order within both groups is preserved.
The transform has no mutable state; the parent resets its per-game state.
No extra observations, quantities, unit actions or model inputs are introduced.
Local C++ source parity, operations and competitive evaluation pending.
Artifact's Kaggle rating is unknown. Apache2.0 attribution and license retained.
''')
    assert name not in registry;registry[name]=str(p.relative_to(ROOT))
registry_path.write_text(json.dumps(registry,indent=2)+'\n')
shutil.copy2(EXP/'league/ahmed_v23/LICENSE.txt',RUN/'LICENSE')
(RUN/'IMPORT.json').write_text(json.dumps({'source':'https://www.kaggle.com/code/ahmedberatozer/notebookd5e3d21fa6',
    'reference':str((donor/'reference_v24.py').relative_to(EXP)),'static_diff':diff,
    'source_change':'Only last entry-point agent function changed; all other36 AST nodes identical to previously verifiedV23.',
    'credits':['AhmedV24 stable premium-sale ordering','yamakawanin KingRC4 inspiration as credited by source','Thomas/yhay81/tetsutani V23 lineage inherited'],
    'local_changes':'Typed stable two-pass order partition; independently vary activation144 or216 on current parent. Control720 never activates.',
    'rating':'Unknown for downloaded artifact; notebook author reports offline tests that are not local evidence.',
    'license':'Apache2.0; retain LICENSE and parent/source credits.'},indent=2)+'\n')
s=(EXP/'runs/yusuke_port_sep08_001/check_operations.py').read_text()
s=s.replace('all four Yusuke component variants','premium-sale ports and ablations')
s=s.replace("variants = [f'yusuke_sep08_m{i}' for i in range(4)]", "variants = ['premium_sales_m0', 'premium_sales_s144', 'premium_sales_s216', 'ahmed_v24']")
s=s.replace("'king_rc4']", "'king_rc4', 'yusuke_sep08_m2', 'investment_context_guarded_001_best']")
s=s.replace("'--pair', 'yusuke_sep08_m2'", "'--pair', 'premium_sales_s216'")
s=s.replace("['binary'], 'yusuke_sep08_m2'", "['binary'], 'premium_sales_s216'")
s=s.replace('Immutable four-tape library; selected route is per instance and reset.', 'Stateless stable order transform; parent retains per-instance reset state.')
(RUN/'check_operations.py').write_text(s)
print('Prepared four premium-sale C++ packages with source lineage.')
