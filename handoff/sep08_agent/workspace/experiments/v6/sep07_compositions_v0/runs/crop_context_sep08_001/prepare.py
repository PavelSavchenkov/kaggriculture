"""Isolate the current agent and expose one crop-family decision per instance."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os
import re
import shlex
import subprocess

RUN = Path(__file__).resolve().parent
EXP = RUN.parents[1]
ROOT = EXP.parents[2]
reference = json.loads((EXP/'CURRENT_REFERENCE.json').read_text())
assert reference['name'] == 'empty_sale_slots_m2'
parent = EXP/reference['path']
manifest = json.loads((parent/'agent.json').read_text())
base = (parent/manifest['header']).resolve()
translations = [(parent/p).resolve() for p in manifest['sources']]
files = set(translations)
commands = []
for path in [base,*translations]:
    command = ['conda','run','-n','kaggriculture','g++','-std=c++20','-I',str(ROOT),'-MM','-MT','closure',str(path)]
    result = subprocess.run(command,capture_output=True,text=True,check=True)
    files.update(Path(p).resolve() for p in shlex.split(result.stdout.replace('\\\n',' ').split(':',1)[1]))
    commands.append(command)
assert all(not p.is_relative_to(ROOT/'experiments') or p.is_relative_to(EXP) for p in files)
local = sorted(p for p in files if p.is_relative_to(EXP))
mapped = {p:RUN/'parent_source'/p.relative_to(EXP) for p in local}
(RUN/'parent_source').mkdir(exist_ok=False)
renames = {'compositions_sale_copy':'compositions_crop_parent',
    'compositions':'compositions_crop_context',
    'kag::agents_sale_copy':'kag::agents_crop_parent',
    'kag::agents':'kag::agents_crop_context'}
for old,new in mapped.items():
    source = old.read_text()
    source = re.sub(r'\b(?:kag::agents_sale_copy|kag::agents|compositions_sale_copy|compositions)\b',
        lambda m:renames[m[0]],source)
    def include(match):
        original = next((p.resolve() for p in [old.parent/match[1],ROOT/match[1]] if p.is_file()),None)
        assert original is not None,(old,match[1])
        return '#include "'+os.path.relpath(mapped.get(original,original),new.parent)+'"'
    source = re.sub(r'#include\s+"([^"]+)"',include,source)
    new.parent.mkdir(parents=True,exist_ok=True)
    new.write_text(source)

prefix = 'runs/observed_sale_lead_003/parent_source/'
modified = []
def edit(relative, replacements):
    path = mapped[EXP/relative]
    source = path.read_text()
    for before,after in replacements:
        assert source.count(before) == 1,(relative,before,source.count(before))
        source = source.replace(before,after)
    path.write_text(source)
    modified.append(relative)

forward = 'public:\n    void set_crop_mode(int mode){base_.set_crop_mode(mode);}\n    bool crop_selected()const{return base_.crop_selected();}\n'
for path in ['runs/observed_sale_lead_004/source/policy.hpp',
    prefix+'runs/rival_wool_context_003/source/policy.hpp',
    prefix+'runs/wool_family_context_v2_001/source/transfer.hpp',
    prefix+'runs/opening_market_search_001/policy/source/agent.hpp']:
    edit(path,[('public:\n',forward)])
edit(prefix+'include/crop_rotation_shop_gate.hpp',[
    ('int minimum_tomatoes_;','int minimum_tomatoes_;\n    int crop_mode_=0;'),
    ('public:\n','public:\n    void set_crop_mode(int mode){crop_mode_=mode;}\n    bool crop_selected()const{return selected_;}\n'),
    ('selected_=demand>=minimum_tomatoes_ && (changed_.matched_days()&(uint32_t(1)<<12));',
     'const bool choose=crop_mode_==0 ? demand>=minimum_tomatoes_ : crop_mode_==2;\n            selected_=choose && (changed_.matched_days()&(uint32_t(1)<<12));'),
])
edit(prefix+'runs/late_portfolio_001/source/policy.hpp',[
    ('int samples_,forced_,choice_=0,selected_day_=-1;','int samples_,forced_,choice_=0,selected_day_=-1;\n    int crop_mode_=0;'),
    ('public:\n','public:\n    void set_crop_mode(int mode){crop_mode_=mode;base_.set_crop_mode(mode);}\n    bool crop_selected()const{return base_.crop_selected();}\n'),
    ('wheat_context_=demand<2;','wheat_context_=crop_mode_==0 ? demand<2 : !base_.crop_selected();'),
])
registry_path = EXP/'configs/league.json'
registry = json.loads(registry_path.read_text())
variants = []
for mode in range(3):
    name = f'crop_context_m{mode}'
    folder = RUN/'proposals'/name
    (folder/'source').mkdir(parents=True,exist_ok=False)
    header = os.path.relpath(mapped[base],folder/'source')
    (folder/'source/agent.hpp').write_text(f'''#pragma once
#include "{header}"
namespace compositions::{name} {{
class Agent:public compositions_crop_context::empty_sale_slots_m2::Agent {{
public:
    Agent(){{set_crop_mode({mode});}}
    static kag::agent::AgentInfo info(){{return {{"{name}"}};}}
}};
}}
''')
    (folder/'source/agent.cpp').write_text('#include "agent.hpp"\n')
    data = {'format_version':1,'name':name,'header':'source/agent.hpp',
        'type':f'compositions::{name}::Agent',
        'sources':['source/agent.cpp',*[os.path.relpath(mapped[p],folder) for p in translations]]}
    (folder/'agent.json').write_text(json.dumps(data,indent=2)+'\n')
    (folder/'README.md').write_text(f'''# {name}

Current empty-sale agent with a per-instance day12 crop-choice parameter.
Mode {mode}: 0 keeps the current shop threshold, 1 prefers productive wheat,
2 prefers the tomato continuation only when its existing entry contract matches.
The day13 animal selector observes the actual crop choice in changed modes.
Mode0 preserves its original context test exactly. All other inherited choices,
worker programs and market transforms remain in the isolated source closure.
See ../../LINEAGE.json for copied origins and changes. Kaggle rating unknown;
this is an experimental external-lineage agent requiring complete validation.
''')
    assert name not in registry
    registry[name] = str(folder.relative_to(ROOT))
    variants.append(name)
registry_path.write_text(json.dumps(registry,indent=2)+'\n')
report = {'created_utc':datetime.now(timezone.utc).isoformat(),'parent':reference,
    'variants':variants,'copied_files':len(local),'modified':modified,'commands':commands,
    'ideas':['Value the wheat/tomato continuation using its full remaining crop calendar and shared market.',
        'First collect matched forced-choice outcomes while preserving inherited farm and sale improvements.',
        'Keep day13 investment eligibility consistent with the actual changed day12 crop choice.'],
    'lineage':'All original replay, notebook and local schedule lineage is inherited through the isolated current source closure. No new external policy copied.',
    'source_hashes':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in local},
    'scope':'Diagnostic forced continuations, not a newly fitted value selector. Exact mode0 controls required.'}
(RUN/'LINEAGE.json').write_text(json.dumps(report,indent=2)+'\n')
print('Prepared',variants,'isolated',len(local),'files; changed',len(modified),'headers.',flush=True)
