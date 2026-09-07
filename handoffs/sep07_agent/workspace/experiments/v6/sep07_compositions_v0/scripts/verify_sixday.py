"""Parity against the original Python/native packages on exogenous observations."""
import hashlib
import importlib.util
import json
import shutil
import subprocess
import sys
from copy import deepcopy
from pathlib import Path
from verify_public_router import pack

EXP=Path(__file__).resolve().parents[1]
ROOT=EXP.parents[2]
ORIGIN=ROOT/"external/kaggriculture/agents/sixday-rl"


def force_branch(obs,step,branch):
    if step==144:
        obs["market"]["prices"]["CARROT"]=33 if branch%2 else 34
        obs["farms"][1-obs["player"]]["tiles"]=[[None]*10 for _ in range(10)]
    if step==432:
        prices=obs["market"]["prices"]
        obs["private"]["shed"]["FERTILIZER"]=0
        prices["TOMATO"]=81 if branch%3==1 else 80
        prices["MILK"]=152 if branch%3==2 else 153
        rival=obs["farms"][1-obs["player"]]
        rival["tiles"]=[[None]*10 for _ in range(10)]
        rival["tiles"][3][3]="WEED"
    if step==576:
        prices=obs["market"]["prices"]
        prices["CARROT"]=66 if branch in (1,3) else 65
        prices["WOOL"]=228 if branch==2 else 227
        prices["STRAWBERRY"]=12
        prices["FERTILIZER"]=12 if branch==4 else 33
        seat=obs["player"]
        obs["farms"][seat]["money"]=obs["farms"][seat^1]["money"]+(4000 if branch==1 else 100)
    if step in (0,1,24,288):
        obs["farms"][obs["player"]]["money"]=(0,149,150,151,900)[branch]


def main():
    reports=[]
    paths=sorted((EXP/"replays").glob("*replay.json"))[:4]
    replays=[json.loads(path.read_text()) for path in paths]
    for mode in range(3):
        target=EXP/f"tests/reference/sixday_{mode}"
        target.mkdir(parents=True,exist_ok=True)
        for filename in ("_sixday_main.py","agent.so"):
            shutil.copy2(ORIGIN/filename,target/filename)
        entry=ORIGIN/("_sixday_main.py" if mode==0 else "main.py")
        if mode==2:entry=ROOT/"external/kaggriculture/agents/sixday-robust-rl/main.py"
        shutil.copy2(entry,target/"main.py")
        hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in target.iterdir() if p.is_file()}
        module_name=f"sixday_reference_{mode}"
        spec=importlib.util.spec_from_file_location(module_name,target/"main.py")
        module=importlib.util.module_from_spec(spec)
        sys.modules[module_name]=module
        spec.loader.exec_module(module)
        fixture=EXP/f"tests/sixday_{mode}_cases.txt"
        with fixture.open("w") as out:
            for replay in replays:
                for seat in range(2):
                    for step,pair in enumerate(replay["steps"][:719]):
                        obs=dict(pair[seat]["observation"],step=step)
                        out.write(pack(obs,module.agent(obs),step==0))
            for branch in range(5):
                for step,pair in enumerate(replays[0]["steps"][:719]):
                    obs=deepcopy(pair[0]["observation"])
                    obs["step"]=step
                    force_branch(obs,step,branch)
                    out.write(pack(obs,module.agent(obs),step==0))
        binary=EXP/f"build/sixday_{mode}_parity"
        command=["conda","run","-n","kaggriculture","g++","-std=c++20","-O2",f"-DVERIFY_SIXDAY={mode}","-I",str(ROOT),
            str(EXP/"tests/public_router_parity.cpp"),str(EXP/"league/teammate_sixday/source/agent.cpp"),"-o",str(binary)]
        subprocess.run(command,check=True)
        result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
        print(mode,result.stdout,result.stderr,flush=True)
        result.check_returncode()
        reports.append({"mode":mode,"result":result.stdout.strip(),"source_sha256":hashes,"command":command,
            "scope":"8 recorded sequences and 5 forced route/low-cash sequences, 719 turns each; active units and semantically used opcode arguments"})
        (EXP/"results/sixday_parity.json").write_text(json.dumps(reports,indent=2)+"\n")


if __name__=="__main__":
    main()
