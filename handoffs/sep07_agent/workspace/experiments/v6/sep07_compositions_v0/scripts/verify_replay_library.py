"""Compare every C++ replay course action with its original source record."""
import json
import subprocess
from pathlib import Path

from verify_public_router import pack

EXP=Path(__file__).resolve().parents[1]
ROOT=EXP.parents[2]


def main():
    metadata=json.loads((EXP/"league/top_replay_library/IMPORT.json").read_text())["programs"]
    fixture=EXP/"tests/replay_library_cases.txt"
    with fixture.open("w") as out:
        for source in metadata:
            replay=json.loads((EXP/f"replays/episode-{source['episode']}-replay.json").read_text())
            seat=source["seat"]
            for step in range(719):
                obs=replay["steps"][step][seat]["observation"];obs["step"]=step
                line=pack(obs,replay["steps"][step+1][seat]["action"],step==0)
                if step==0:line=str(source["program"]+1)+" "+line.split(" ",1)[1]
                out.write(line)
    binary=EXP/"build/replay_library_parity"
    command=["conda","run","-n","kaggriculture","g++","-std=c++20","-O2","-DVERIFY_LIBRARY","-I",str(ROOT),
        str(EXP/"tests/public_router_parity.cpp"),str(EXP/"league/top_replay_library/source/agent.cpp"),"-o",str(binary)]
    subprocess.run(command,check=True)
    result=subprocess.run([str(binary),str(fixture)],check=True,capture_output=True,text=True)
    report={"result":result.stdout.strip(),"programs":len(metadata),
        "scope":"All 719 source actions for every course; active worker count normalized", "command":command}
    (EXP/"results/replay_library_parity.json").write_text(json.dumps(report,indent=2)+"\n")
    print(result.stdout.strip())


if __name__=="__main__":main()
