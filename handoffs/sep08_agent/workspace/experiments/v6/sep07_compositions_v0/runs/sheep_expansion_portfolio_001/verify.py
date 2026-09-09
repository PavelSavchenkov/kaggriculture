"""Check fixed-source actions and force both declared Yarn-selector routes."""
import json
import subprocess
from pathlib import Path

RUN=Path(__file__).resolve().parent
ROOT=RUN.parents[4]
ENV=['conda','run','-n','kaggriculture']


def main():
    (RUN/'build').mkdir(exist_ok=True);binary=RUN/'build/parity'
    build=ENV+['g++','-std=c++20','-O2','-I',str(ROOT),str(RUN/'tests/parity.cpp'),str(RUN/'source/agent.cpp'),'-o',str(binary)]
    subprocess.run(build,check=True);reports=[]
    for branch in range(2):
        for mode in [branch,2,3]:
            command=ENV+[str(binary),str(RUN/f'tests/source_{branch}.txt'),str(mode),str(branch),str(RUN/f'tests/source_state_{branch}.json')]
            result=subprocess.run(command,capture_output=True,text=True,check=True);print(branch,mode,result.stdout,flush=True);reports.append({'command':command,'result':result.stdout.strip()})
    (RUN/'SOURCE_PARITY.json').write_text(json.dumps({'build':build,'reports':reports,'scope':'Two719-action original courses plus each forced Yarn2/Yarn3 branch on the same exogenous original observations. Local selector formulas are not claimed as donor source.'},indent=2)+'\n')


if __name__=='__main__':main()
