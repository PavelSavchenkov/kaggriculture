"""Source parity on exogenous full observations, including forced branches."""
import argparse
import importlib.util
import json
import subprocess
from collections import Counter
from copy import deepcopy
from pathlib import Path
from verify_public_router import pack

EXP=Path(__file__).resolve().parents[1]
CONFIG={"townCenterSellInterval":24,"townShopSellInterval":4,"turnsPerDay":24,"shedCapacity":100}
BRANCHES=("backbone","yarn","pet","farmers","smoothie","recovery","recovery_smoothie",
    "recovery_bakery","recovery_ice","ice","bakery_yarn","pizza","known_yarn_a","known_yarn_b","clone","market_stress")


def assets(farm,cows=3):
    tiles=[[None]*10 for _ in range(10)]
    objects=[{"kind":"PASTURE","animal":"COW"} for _ in range(cows)]
    objects += [{"kind":"PASTURE","animal":"SHEEP"} for _ in range(2)]
    objects += [{"kind":"PLANT","crop":"WHEAT"} for _ in range(7)]
    objects += [{"kind":"PLANT","crop":"MELON"} for _ in range(12)]
    for i,tile in enumerate(objects):tiles[i//10][i%10]=tile
    farm["tiles"]=tiles


def force(obs,branch):
    step=obs["step"];seat=obs["player"]
    first={"yarn":"YARN_STORE","pet":"PET_CAFE","farmers":"FARMERS_MARKET","smoothie":"FARMERS_MARKET",
        "recovery_smoothie":"SMOOTHIE_SHOP","recovery_ice":"ICE_CREAM_SHOP","ice":"ICE_CREAM_SHOP",
        "pizza":"PIZZA_SHOP","known_yarn_a":"YARN_STORE","known_yarn_b":"YARN_STORE","clone":"YARN_STORE"}.get(branch,"BAKERY")
    second={"pet":"YARN_STORE","smoothie":"SMOOTHIE_SHOP","bakery_yarn":"YARN_STORE","pizza":"PET_CAFE"}.get(branch,"BAKERY")
    obs["town"]["unlocked_shops"]=[first,second][:0 if step<72 else 1 if step<144 else 2]
    if step==72:
        own,rival=obs["farms"][seat],obs["farms"][seat^1]
        own["money"]=10000;rival["money"]=9000
        signature={"farmers":(141,193,9974),"smoothie":(141,193,9974),"recovery":(145,195,9975),
            "recovery_smoothie":(145,193,9975),"recovery_bakery":(142,142,9973),"recovery_ice":(142,191,9974),
            "ice":(145,195,9975),"bakery_yarn":(141,193,9974),"pizza":(141,193,9974)}.get(branch)
        if signature:
            own["money"],rival["money"],obs["market"]["inventory"]["WHEAT"]=signature
            assets(rival)
    if step==96 and first=="YARN_STORE":
        own,rival=obs["farms"][seat],obs["farms"][seat^1]
        own["money"]=10000;rival["money"]=9000
        if branch in ("known_yarn_a","known_yarn_b"):
            signature=(214,214,9982) if branch.endswith("a") else (215,125,9979)
            own["money"],rival["money"],obs["market"]["inventory"]["WHEAT"]=signature
            assets(rival,4)
    if branch=="clone" and step>=72:
        obs["farms"][seat^1]=deepcopy(obs["farms"][seat])
    if branch=="market_stress" and step>=100:
        own,rival=obs["farms"][seat],obs["farms"][seat^1]
        own["money"]=5000;rival["money"]=15000
        obs["private"]["shed"]={item:12 for item in ("CARROT","TOMATO","STRAWBERRY","MELON","EGG","MILK","WOOL")}
        for item in obs["private"]["shed"]:
            obs["market"]["inventory"][item]=10000+(step%3)*40
            obs["market"]["prices"][item]=200


def fail_fallback(obs):
    raise RuntimeError(f"Unexpected source fallback at step {obs['step']}")


def main():
    parser=argparse.ArgumentParser();parser.add_argument("--quick",action="store_true");args=parser.parse_args()
    source=EXP/"research/kaito_v58/upstream.py"
    spec=importlib.util.spec_from_file_location("kaito_v58_reference",source)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    module._v51_safe_action=fail_fallback
    paths=sorted((EXP/"replays").glob("*replay.json"))[:1 if args.quick else 4]
    fixture=EXP/("tests/kaito_v58_quick.txt" if args.quick else "tests/kaito_v58_cases.txt")
    modes=Counter();calls=0
    with fixture.open("w") as out:
        for path in paths:
            replay=json.loads(path.read_text())
            for seat in range(1 if args.quick else 2):
                for step,pair in enumerate(replay["steps"][:719]):
                    obs=dict(pair[seat]["observation"],step=step)
                    action=module.agent(obs,CONFIG)
                    out.write(pack(obs,action,step==0,full_tiles=True));calls+=1
                print("recorded",path.name,seat,calls,flush=True)
        if not args.quick:
            replay=json.loads(paths[0].read_text())
            for branch in BRANCHES:
                for step,pair in enumerate(replay["steps"][:719]):
                    obs=deepcopy(pair[0]["observation"]);obs["step"]=step
                    force(obs,branch)
                    action=module.agent(obs,CONFIG)
                    state=module._V58_STATES[0]
                    modes[str((state.get("mode"),state.get("known_yarn"),state.get("clone_selected")))]+=1
                    out.write(pack(obs,action,step==0,full_tiles=True));calls+=1
                print("forced",branch,calls,flush=True)
    result=subprocess.run([str(EXP/"build/kaito_v58_parity"),str(fixture)],text=True,capture_output=True)
    print(result.stdout,result.stderr,flush=True);result.check_returncode()
    telemetry={name:policy.telemetry for name,policy in module._V58_POLICIES.items()}
    report={"result":result.stdout.strip(),"cases":calls,"forced_modes":dict(modes),"controllers":telemetry,
        "scope":"Full tile fields, legal public/private boundary, active units and semantic opcode arguments; exogenous observations, no Python game simulation"}
    output=EXP/("results/kaito_v58_quick.json" if args.quick else "results/kaito_v58_parity.json")
    output.write_text(json.dumps(report,indent=2)+"\n")


if __name__=="__main__":main()
