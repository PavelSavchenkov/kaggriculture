"""Record checked runner equivalence for the new public and replay packages."""
import hashlib
import json
from pathlib import Path

EXP=Path(__file__).resolve().parents[1]


def read(path):
    return json.loads((EXP/path).read_text())


def main():
    pairs=(("kaito_v58_checks/kaito_v58_vs_opening_router_v1.json","kaito_v58_debug.json"),
        ("fresh_counter_league/junghoon_78_vs_mao_85.json","junghoon_mao85_debug.json"),
        ("fresh_counter_league/mao_89_vs_pass.json","mao89_pass_debug.json"))
    report=[]
    for generic_path,debug_path in pairs:
        generic=read("results/"+generic_path);debug=read("results/"+debug_path)
        assert generic["games"][:len(debug["games"])]==debug["games"],debug_path
        report.append({"generic":generic_path,"debug":debug_path,"identical_games":len(debug["games"])})
    for name in ("kaito_v58","junghoon_78","mao_85","mao_89"):
        directory="kaito_v58_checks" if name=="kaito_v58" else "fresh_counter_league"
        solo=read(f"results/{directory}/{name}_vs_pass.json")
        selfplay=read(f"results/{directory}/{name}_vs_{name}.json")
        assert selfplay["win_utility"]==.5 and selfplay["mean_margin"]==0
        report.append({"agent":name,"pass_games":len(solo["games"]),"pass_J":solo["pass_J"],
            "self_games":len(selfplay["games"]),"self_utility":selfplay["win_utility"]})
    (EXP/"results/audited_ports_validation.json").write_text(json.dumps(report,indent=2)+"\n")
    target=EXP/"league/kaito_v58";path=target/"IMPORT.json";data=json.loads(path.read_text())
    data["status"]="17,256 source actions match; generic/debug/thread, PASS/self and 13-opponent discovery completed"
    data["port_sha256"]={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((target/"source").iterdir())}
    data["validation"]="results/kaito_v58_parity.json and results/audited_ports_validation.json"
    path.write_text(json.dumps(data,indent=2)+"\n")
    print(json.dumps(report,indent=2))


if __name__=="__main__":main()
