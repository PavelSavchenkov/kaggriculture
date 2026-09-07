"""Check deployment parity and report the schedule edit's measured scope."""
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]


def read(path):
    return json.loads((EXP/path).read_text())


def main():
    generic=read("results/opening_router_v1_checks/opening_router_v1_vs_opening_router_v0.json")
    debug=read("results/opening_router_v1_debug.json")
    assert generic["games"]==debug["games"]
    advance=read("results/advance_sales_7_debug.json")
    discovery=read("results/advance_sales_discovery/advance_sales_001_7_vs_feeltheagi_55.json")
    assert advance["games"]==discovery["games"][:16]
    profile=read("results/opening_router_v1_profile.json")
    parent=read("results/teacher55_profile/program_55_teacher.json")
    changes={}
    for new,old in zip(profile["games"],parent["games"],strict=True):
        assert (new["seed"],new["seat"])==(old["seed"],old["seat"])
        for key in old:
            if new[key]!=old[key]:changes[key]=changes.get(key,0)+1
        assert new["produced"]==old["produced"]
        assert new["discarded"]==old["discarded"]
        normalize=lambda lives: sorted((l[0],l[1],l[2],l[3]//24,l[4]//24,*l[5:]) for l in lives)
        assert normalize(new["profile"]["lives"])==normalize(old["profile"]["lives"])
        for key in ("hires","hire_cost","land","land_cost","buys"):
            assert new["profile"][key]==old["profile"][key],key
    reports={}
    for directory in ("opening_router_v1_promotion","opening_router_v1_promotion_remaining"):
        for path in sorted((EXP/"results"/directory).glob("*_vs_*.json")):
            data=json.loads(path.read_text())
            reports[path.stem]={k:v for k,v in data.items() if k!="games"}
    report={"generic_debug_thread_games":len(generic["games"]),
        "advance_generic_debug_games":len(advance["games"]),
        "paired_profile_games":len(profile["games"]),"profile_changed_fields":changes,
        "unchanged":"production, discards, daily service/lifetimes, workforce, land and purchases in all 32 profiled games; intraday work and lifecycle endpoints move",
        "promotion":reports}
    (EXP/"results/opening_router_v1_validation.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report,indent=2))


if __name__=="__main__":main()
