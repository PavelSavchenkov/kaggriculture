"""Export original full-state fixtures and replay both tapes in C++ only."""
import hashlib
import json
import subprocess
import sys
from pathlib import Path

RUN=Path(__file__).resolve().parent
EXP=RUN.parents[1]
ROOT=EXP.parents[2]
sys.path.insert(0,str(EXP/'scripts'))
from verify_public_router import pack


def main():
    folder=RUN/'donor_replay';folder.mkdir(exist_ok=True)
    commands=[];sources={};reports=[]
    for branch,label in enumerate(['small','expansion']):
        donor=json.loads((EXP/f'research/refresh_1342/sheep_expansion/{label}/IMPORT.json').read_text())['source']
        path=EXP/donor['replay_file'];sources[str(path.relative_to(EXP))]=hashlib.sha256(path.read_bytes()).hexdigest();r=json.loads(path.read_text())
        assert r['configuration']['seed'] is None
        fixture=folder/f'{label}.txt'
        with fixture.open('w')as f:
            for step in range(720):
                for seat in range(2):
                    action=r['steps'][step+1][seat]['action'] if step<719 else {'farmer':['PASS'],'hands':[],'market':[]}
                    f.write(pack(r['steps'][step][seat]['observation'],action,step==0,True))
        binary=folder/'replay_check'
        if not branch:
            command=['conda','run','-n','kaggriculture','g++','-std=c++20','-O2','-DKAG_VERIFY_MASKS','-I',str(ROOT),str(RUN/'source/donor_replay_check.cpp'),str(RUN/'source/agent.cpp'),'-o',str(binary)]
            subprocess.run(command,check=True);commands.append(command)
        command=['conda','run','-n','kaggriculture',str(binary),str(fixture),str(branch),str(donor['seat']),str(folder/f'{label}_result.json')]
        subprocess.run(command,check=True);commands.append(command);reports.append(json.loads((folder/f'{label}_result.json').read_text()))
    result={'scope':'Original complete two-player recorded action courses replayed in C++. Original seed unavailable. Only externally observed shop identities and new day-start weeds are imposed; no cash, market inventory, farm biology or carried stock is corrected. All own farm states, inventories, money and shared market compared before all720states. Serialized inventory key order excluded from comparison; natural runtime order remains in simulation.','source_sha256':sources,'commands':commands,'reports':reports,'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()}
    (RUN/'DONOR_REPLAY_CHECK.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))


if __name__=='__main__':main()
