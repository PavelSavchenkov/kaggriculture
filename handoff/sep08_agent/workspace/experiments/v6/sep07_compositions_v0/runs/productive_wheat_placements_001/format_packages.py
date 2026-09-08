# Adapted from productive_wheat_rotation_001/format_packages.py; root collection after worker usage limit.
"""Format complete checked wheat policies without changing any catalog."""
import hashlib
import json
import os
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
NAMES=[row["name"] for row in json.loads((RUN/"PROPOSALS.json").read_text())]


def main():
    for name in NAMES:
        source=name;package=RUN/'proposals'/name
        assert not package.exists();(package/'source').mkdir(parents=True)
        relative=lambda p:os.path.relpath(p,package/'source')
        code='#pragma once\n#include "'+relative(EXP/'include/crop_branch_sequence.hpp')+'"\n#include "'+relative(EXP/'runs/crop_value_001/proposals/crop_value_m2_t4/source/agent.hpp')+'"\n'
        folders={key:RUN/(source+suffix)for key,suffix in [('off',''),('on','_berry')]}
        hashes={}
        for key,folder in folders.items():
            status=json.loads((folder/'STATUS.json').read_text());assert status['status']=='compiled'and status['days']==16
            for day in range(13,29):code+='#include "'+relative(folder/f'days/{day}/schedule.hpp')+'"\n'
        code+=f'namespace compositions::{name}{{\n'
        for key,folder in folders.items():
            code+=f'inline std::vector<GuardedDay> {key}_days(){{std::vector<GuardedDay>result;\n'
            for day in range(13,29):
                path=folder/f'days/{day}';values=[list(map(int,line.split()))for line in (path/'guard.txt').read_text().splitlines()]
                assert len(values)==103 and all(len(row)==13 for row in values[1:101]);assert values[0][0]==day
                code+=f'{{GuardedDay g;g.plan={{{day},{folder.name}_d{day}::schedule()}};g.quadrants={values[0][1]};\ng.tiles={{{{'
                code+=','.join('{{'+','.join(map(str,row[1:]))+'}}'for row in values[1:101])+'}};\n'
                for field,data in [('check',[row[0]for row in values[1:101]]),('shed',values[101]),('seeds',values[102])]:code+=f'g.{field}={{{",".join(map(str,data))}}};\n'
                code+='result.push_back(std::move(g));}\n'
                for p in [path/'guard.txt',path/'schedule.hpp',path/'problem.json']:hashes[str(p.relative_to(EXP))]=hashlib.sha256(p.read_bytes()).hexdigest()
            code+='return result;}\n'
        preserve='true'
        code+=f'class Agent:public CropBranchSequence<crop_value_m2_t4::Agent>{{public:Agent():CropBranchSequence(off_days(),on_days()){{}}static kag::agent::AgentInfo info(){{return {{"{name}"}};}}}};}}\n'
        (package/'source/agent.hpp').write_text(code);(package/'source/agent.cpp').write_text('#include "agent.hpp"\n')
        (package/'agent.json').write_text(json.dumps({'format_version':1,'name':name,'header':'source/agent.hpp','type':f'compositions::{name}::Agent','sources':['source/agent.cpp',os.path.relpath(EXP/'league/top_replay_library/source/agent.cpp',package)]},indent=2)+'\n')
        (package/'IMPORT.json').write_text(json.dumps({'parent':'crop_value_m2_t4','donor_lineage':'../../UPSTREAM_LINEAGE.json','source_courses':{k:str(v.relative_to(RUN))for k,v in folders.items()},'preserve_parent_berry_branch':preserve=='true','scope':'Complete checked day13..28 wheat replacement; legal day20 berry-demand branch selects separately compiled continuation. Physical guards do not guarantee future funds. Unpromoted.','source_sha256':hashes},indent=2)+'\n')
        (package/'README.md').write_text(f'# {name}\n\nComplete wheat calendar over crop_value_m2_t4; see ../../PLAN.md and IMPORT.json. Both day20 berry continuations are compiled and have an identical entry guard. This policy {"preserves"if preserve=="true"else"deliberately disables"} the parent\'s optional berry branch after entering the wheat course. Validation pending; no promotion or submission.\n')
    print('Formatted',len(NAMES),'isolated packages.')


if __name__=='__main__':main()
