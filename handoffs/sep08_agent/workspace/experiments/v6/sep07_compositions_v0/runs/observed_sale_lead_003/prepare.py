"""Isolate the incumbent closure and share only immutable day-plan vectors."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os
import re
import shlex
import subprocess

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
OLD=EXP/'runs/observed_sale_lead_002'
base=EXP/'runs/rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp'
translation=[EXP/'league/top_replay_library/source/agent.cpp',EXP/'league/public_router/source/agent.cpp']
files=set()
commands=[]
for p in [base,*translation]:
    command=['conda','run','-n','kaggriculture','g++','-std=c++20','-I',str(ROOT),'-MM','-MT','closure',str(p)]
    result=subprocess.run(command,capture_output=True,text=True,check=True)
    deps=shlex.split(result.stdout.replace('\\\n',' ').split(':',1)[1])
    files.update(Path(v).resolve() for v in deps)
    commands.append(command)
files.update(translation)
local=sorted(p for p in files if p.is_relative_to(EXP))
for p in files:
    assert not p.is_relative_to(ROOT/'experiments') or p.is_relative_to(EXP)
mapped={p:RUN/'parent_source'/p.relative_to(EXP) for p in local}
shared=RUN/'source/shared_vector.hpp'
shared.parent.mkdir(exist_ok=True)
shared.write_text('''#pragma once
#include <memory>
#include <vector>
#include <cstdlib>
namespace compositions_sale_copy {
// Read-only after construction. Only the original constructor appends entries;
// appending after the object has been copied fails instead of mutating peers.
template<class T> class SharedVector {
    std::shared_ptr<std::vector<T>> values_;
public:
    SharedVector():values_(std::make_shared<std::vector<T>>()){}
    SharedVector(std::vector<T> values):values_(std::make_shared<std::vector<T>>(std::move(values))){}
    bool empty()const{return values_->empty();}
    size_t size()const{return values_->size();}
    const T& operator[](size_t i)const{return (*values_)[i];}
    auto begin()const{return values_->cbegin();}
    auto end()const{return values_->cend();}
    void push_back(const T& value){if(!values_.unique())std::abort();values_->push_back(value);}
};
}
''')
modified=[]
for old,new in mapped.items():
    text=old.read_text()
    text=re.sub(r'\bcompositions\b','compositions_sale_copy',text)
    text=text.replace('kag::agents::','kag::agents_sale_copy::')
    pattern=r'std::vector<(GuardedDay|DayPlan)>\s+(days_|plans_|off_\s*,\s*on_)\s*;'
    text,n=re.subn(pattern,lambda m:f'SharedVector<{m[1]}> {m[2]};',text)
    if n:
        modified.append({'path':str(old.relative_to(EXP)),'members':n})
        text='#include "'+os.path.relpath(shared,new.parent)+'"\n'+text
    def include(match):
        value=match[1]
        if value==os.path.relpath(shared,new.parent):return match[0]
        candidates=[old.parent/value,ROOT/value]
        original=next((p.resolve() for p in candidates if p.is_file()),None)
        assert original is not None,(old,value)
        target=mapped.get(original,original)
        return '#include "'+os.path.relpath(target,new.parent)+'"'
    text=re.sub(r'#include\s+"([^"]+)"',include,text)
    new.parent.mkdir(parents=True,exist_ok=True)
    new.write_text(text)
assert len(modified)>=5,modified

policy=(OLD/'source/policy.hpp').read_text()
policy=policy.replace('namespace compositions::observed_sale_lead_fast','namespace compositions::observed_sale_lead_shared')
policy=policy.replace('#include "../../../runs/rival_wool_context_003/proposals/rival_wool_context_v3/source/agent.hpp"',
                      '#include "'+os.path.relpath(mapped[base],RUN/'source')+'"')
policy=policy.replace('using Base=kag::agents::rival_wool_context_v3::Agent;', 'using Base=kag::agents_sale_copy::rival_wool_context_v3::Agent;')
(RUN/'source/policy.hpp').write_text(policy)
mapping={'sale_lead_fast_control':'sale_lead_shared_control','observed_sale_lead_fast_v2':'observed_sale_lead_shared_v3',
         'observed_sale_lead_fast_milk_wool':'observed_sale_lead_shared_milk_wool','compositions::observed_sale_lead_fast':'compositions::observed_sale_lead_shared'}
register=(OLD/'register.py').read_text()
for a,b in mapping.items():register=register.replace(a,b)
register=register.replace("'../../../../league/top_replay_library/source/agent.cpp','../../../../league/public_router/source/agent.cpp'",
                        ','.join(repr(os.path.relpath(mapped[p],RUN/'proposals/agent')) for p in translation))
(RUN/'register.py').write_text(register)
subprocess.run(['conda','run','-n','kaggriculture','python',str(RUN/'register.py')],check=True)
for name in ['run_discovery.py','forecast_probe.cpp','run_probe.py']:
    text=(OLD/name).read_text()
    for a,b in mapping.items():text=text.replace(a,b)
    if name=='run_probe.py':
        for p in translation:text=text.replace("str(EXP/'"+str(p.relative_to(EXP))+"')", "str(RUN/'"+str(mapped[p].relative_to(RUN))+"')")
        # The probe's rival is the unchanged parent, so link its two original
        # implementation files as well as the copied parent's implementations.
        text=text.replace("'-o',str(binary)",','.join("str(EXP/'"+str(p.relative_to(EXP))+"')" for p in translation)+",'-o',str(binary)")
    (RUN/name).write_text(text)
report={'created_utc':datetime.now(timezone.utc).isoformat(),'source_files':len(local),'modified':modified,
        'namespace_change':['compositions to compositions_sale_copy','kag::agents to kag::agents_sale_copy'],
        'copied_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest() for p in local},
        'shared_vector_sha256':hashlib.sha256(shared.read_bytes()).hexdigest(),
        'commands':commands,'scope':'No root engine/API or day_solver copies. Exact parent source closure isolated; only immutable calendar member storage changes. Other episode-state vectors remain private and copied.',
        'required':'Full source-parent control and all completed sale-lead prototype records identical; broad/native/operational checks before promotion.'}
(RUN/'COPY_OPTIMIZATION.json').write_text(json.dumps(report,indent=2)+'\n')
print('Isolated',len(local),'files; shared calendar members in',len(modified),'headers.')
