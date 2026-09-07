"""Source parity for the two capacity/terminal router variants."""
import importlib.util
import json
import subprocess
from collections import Counter
from copy import deepcopy
from pathlib import Path
from verify_public_router import pack

EXP=Path(__file__).resolve().parents[1]
ROOT=EXP.parents[2]


def main():
    sources=(EXP/"research/notebooks/tetsutani/shape-the-shop-work-the-pasture-kaggriculture/extracted_MAIN_B64.py",
        EXP/"research/refresh_0504/notebooks/lynnsakurai/farming-score-a-mathematical-approach/extracted_MAIN_B64.py")
    replays=[json.loads(p.read_text()) for p in sorted((EXP/"replays").glob("*replay.json"))[:4]]
    report=[]
    for mode,source in enumerate(sources):
        spec=importlib.util.spec_from_file_location(f"capacity_source_{mode}",source)
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        fixture=EXP/f"tests/capacity_{mode}_cases.txt";routes=Counter();cases=0
        with fixture.open("w") as out:
            for replay in replays:
                for seat in range(2):
                    policy=module.Agent()
                    for step,pair in enumerate(replay["steps"][:719]):
                        obs=dict(pair[seat]["observation"],step=step)
                        action=policy.act(obs);routes[policy.cur]+=1;cases+=1
                        out.write(pack(obs,action,step==0,full_tiles=True))
            for branch in range(4):
                policy=module.Agent()
                for step,pair in enumerate(replays[0]["steps"][:719]):
                    obs=deepcopy(pair[0]["observation"]);obs["step"]=step
                    if step==226:obs["town"]["unlocked_shops"]=["YARN_STORE" if branch in (1,2) else "BAKERY"]
                    if step==360:obs["market"]["prices"]["CARROT"]=42 if branch==2 else 41
                    if step==433:obs["market"]["inventory"]["MILK"]=10067 if branch==3 else 10066
                    if step%24==23 or step==718:
                        obs["private"]["shed"]={item:11 for item in module.PRODUCTS}
                        obs["private"]["inventories"]=[{"WOOL":3,"FERTILIZER":2} for _ in obs["private"]["inventories"]]
                    if step==718:
                        obs["market"]["prices"]={item:1 for item in module.PRODUCTS}
                    action=policy.act(obs);routes[policy.cur]+=1;cases+=1
                    out.write(pack(obs,action,step==0,full_tiles=True))
        binary=EXP/f"build/capacity_{mode}_parity"
        command=["conda","run","-n","kaggriculture","g++","-std=c++20","-O2",f"-DVERIFY_CAPACITY={mode}","-I",str(ROOT),
            str(EXP/"tests/public_router_parity.cpp"),str(EXP/"league/public_capacity_router/source/agent.cpp"),"-o",str(binary)]
        subprocess.run(command,check=True)
        result=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
        print(mode,result.stdout,result.stderr,flush=True);result.check_returncode()
        assert len(routes)==4,routes
        report.append({"terminal":bool(mode),"cases":cases,"result":result.stdout.strip(),"route_actions":dict(routes),
            "scope":"8 recorded +4 forced branch/capacity/floor-price-terminal full sequences, full public tiles"})
        (EXP/"results/capacity_router_parity.json").write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":main()
