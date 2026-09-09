"""Run a bounded C++ state trace for the two missed seed1004 placements."""
import hashlib
import json
import subprocess
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]


def main():
    source=[RUN/'source/missing_sheep.cpp',RUN/'source/agent.cpp',EXP/'league/teammate_shoprouter/source/agent.cpp'];binary=RUN/'build/missing_sheep'
    command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O2','-I',str(ROOT),*map(str,source),'-o',str(binary)];subprocess.run(command,check=True)
    run=['conda','run','-n','kaggriculture',str(binary),str(RUN/'missing_sheep_trace.json')];subprocess.run(run,check=True)
    rows=json.loads((RUN/'missing_sheep_trace.json').read_text());events=[{'step':r['step'],'money':r['money'],'shed_sheep':r['shed_sheep'],'cell':c}for r in rows for c in r['cells']if any(u[1] in [7,12,14]for u in c['units'])]
    report={'commands':[command,run],'source_sha256':{str(p.relative_to(EXP)):hashlib.sha256(p.read_bytes()).hexdigest()for p in source},'seed':1004,'seat':0,'opponent':'teammate_shoprouter','events':events,'unit_fields':['unit','requested_opcode','accepted_opcode','carried_sheep']}
    (RUN/'MISSING_SHEEP.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))


if __name__=='__main__':main()
