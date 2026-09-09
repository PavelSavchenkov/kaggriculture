"""Compile isolated dependency-frozen generic and typed/debug C++ arenas."""
import argparse
import hashlib
import json
import shlex
import subprocess
from pathlib import Path
from format_packages import NAMES

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--debug',action='store_true');args=parser.parse_args()
    build=RUN/'build'/('policies_debug'if args.debug else'policies');build.mkdir(exist_ok=True)
    catalog=json.loads((EXP/'configs/league.json').read_text())
    rivals=['crop_value_m2_t4','teammate_shoprouter','public_router_v5','king_rc4','public_router']
    packages={name:ROOT/catalog[name]for name in rivals}
    packages.update({name:RUN/'proposals'/name for name in NAMES})
    types={'pass':'compositions::Pass'};sources=[];code='#pragma once\n#include <memory>\n#include <string>\n#include "evaluation.hpp"\n'
    for name,package in packages.items():
        manifest=json.loads((package/'agent.json').read_text());types[name]=manifest['type']
        code+=f'#include "{package.relative_to(ROOT)/manifest["header"]}"\n'
        sources.extend((package/p).resolve()for p in manifest['sources'])
    if args.debug:code+='using PairA=compositions::wheat_one_fert::Agent;\nusing PairB=compositions::wheat_three_fert::Agent;\n'
    else:
        code+='''struct AnyAgent {uint32_t matched=0;bool berry=false;virtual ~AnyAgent()=default;virtual void reset(const kag::agent::AgentInit&)=0;virtual void act(const kag::agent::AgentObservation&,const kag::agent::DecisionBudget&,kag::Action&)=0;};
template<class T>struct Model:AnyAgent{T value;void reset(const kag::agent::AgentInit&i)override{value.reset(i);matched=0;berry=false;}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a)override{value.act(o,b,a);if constexpr(requires{value.matched_days();value.berry_selected();}){matched=value.matched_days();berry=value.berry_selected();}}};
struct Box{std::unique_ptr<AnyAgent>value;void reset(const kag::agent::AgentInit&i){value->reset(i);}void act(const kag::agent::AgentObservation&o,const kag::agent::DecisionBudget&b,kag::Action&a){value->act(o,b,a);}};
inline Box make_agent(const std::string&name){
'''
        for name,kind in types.items():code+=f'if(name=="{name}")return {{std::make_unique<Model<{kind}>>()}};\n'
        code+='std::abort();}\n'
    (build/'registry.hpp').write_text(code)
    flags=['-std=c++20','-march=native','-mtune=native','-fno-exceptions','-fno-rtti','-pthread']
    flags+=['-O1','-g','-DKAG_VERIFY_MASKS','-DCOMPOSITIONS_PAIR']if args.debug else['-O3','-DNDEBUG','-flto']
    sources=sorted(set(sources));mains=[EXP/'src/arena.cpp']+([]if args.debug else[RUN/'source/diagnostics.cpp'])
    includes=['-I',str(ROOT),'-I',str(EXP/'include'),'-I',str(build)]
    dependencies=subprocess.check_output(['conda','run','-n','kaggriculture','g++',*flags,*includes,'-MM',*map(str,mains+sources)],text=True)
    paths=set(mains+sources)
    for line in dependencies.replace('\\\n',' ').splitlines():
        if line.strip():paths.update(Path(p).resolve()for p in shlex.split(line.split(':',1)[1]))
    snapshot=build/'source_snapshot';hashes={}
    for path in sorted(paths):
        relative=path.relative_to(ROOT);target=snapshot/relative;target.parent.mkdir(parents=True,exist_ok=True);data=path.read_bytes();target.write_bytes(data);hashes[str(relative)]=hashlib.sha256(data).hexdigest()
    includes=['-I',str(snapshot),'-I',str(snapshot/EXP.relative_to(ROOT)/'include'),'-I',str(snapshot/build.relative_to(ROOT))]
    commands=[]
    for main in mains:
        target=build/('arena'if main.name=='arena.cpp'else'diagnostics')
        command=['conda','run','-n','kaggriculture','g++',*flags,*includes,*[str(snapshot/p.relative_to(ROOT))for p in [main,*sources]],'-o',str(target)]
        subprocess.run(command,check=True);commands.append({'command':command,'binary_sha256':hashlib.sha256(target.read_bytes()).hexdigest()})
    (build/'BUILD.json').write_text(json.dumps({'source_sha256':hashes,'binaries':commands},indent=2)+'\n');print(build)


if __name__=='__main__':main()
