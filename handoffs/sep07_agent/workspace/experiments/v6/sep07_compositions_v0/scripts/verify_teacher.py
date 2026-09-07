"""Verify the C++ recorded execution reference against all original actions."""
import json
import subprocess
from pathlib import Path

from verify_public_router import pack

EXP=Path(__file__).resolve().parents[1]
ROOT=EXP.parents[2]


def main():
    metadata=json.loads((EXP/"league/leader_program0_tape/IMPORT.json").read_text())
    replay=json.loads((EXP/f"replays/episode-{metadata['episode']}-replay.json").read_text())
    fixture=EXP/"tests/leader_program0_cases.txt"
    with fixture.open("w") as out:
        for step in range(719):
            seat=metadata["seat"]
            obs=replay["steps"][step][seat]["observation"]
            obs["step"]=step
            out.write(pack(obs,replay["steps"][step+1][seat]["action"],step==0))
    binary=EXP/"build/teacher_parity"
    command=["conda","run","-n","kaggriculture","g++","-std=c++20","-O2","-DVERIFY_TEACHER","-I",str(ROOT),
        str(EXP/"tests/public_router_parity.cpp"),str(EXP/"league/leader_program0_tape/source/agent.cpp"),"-o",str(binary)]
    subprocess.run(command,check=True)
    result=subprocess.run([str(binary),str(fixture)],check=True,capture_output=True,text=True)
    report={"result":result.stdout.strip(),"scope":"All 719 source actions; current worker count normalized", "command":command}
    (EXP/"results/teacher_parity.json").write_text(json.dumps(report,indent=2)+"\n")
    print(result.stdout.strip())


if __name__=="__main__":main()
