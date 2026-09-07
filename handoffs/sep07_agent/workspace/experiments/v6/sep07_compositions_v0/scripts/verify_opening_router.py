"""Verify the new branch preserves the selected parent's complete behavior."""
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]
RESULTS=EXP/"results"


def read(name):return json.loads((RESULTS/(name+".json")).read_text())


def main():
    debug=read("opening_router_debug");generic=read("opening_router_generic_smoke")
    assert debug["games"]==generic["games"]
    keys=["seed","seat","cash","opponent_cash","turns","action_hash","opponent_action_hash","worker_days","produced","sold","discarded"]
    reports=[]
    for name in ["skomuro","deniz","arman_3000","indar","roman","teammate_shoprouter"]:
        new=read("opening_router_"+name+"_discovery")
        old=read(("teacher55_" if name=="teammate_shoprouter" else "public_router_")+name+"_discovery")
        assert len(new["games"])==len(old["games"])
        for a,b in zip(new["games"],old["games"]):
            for key in keys:assert a[key]==b[key],(name,a["seed"],a["seat"],key)
        reports.append({"opponent":name,"games":len(new["games"]),"same_parent_behavior":True})
    report={"generic_debug_thread_parity_games":len(debug["games"]),"parent_comparisons":reports}
    (RESULTS/"opening_router_parent_parity.json").write_text(json.dumps(report,indent=2)+"\n")
    print(json.dumps(report))


if __name__=="__main__":main()
