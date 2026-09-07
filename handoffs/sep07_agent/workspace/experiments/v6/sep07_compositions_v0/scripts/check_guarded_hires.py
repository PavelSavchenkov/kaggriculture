"""Check exact guard comparisons and attribute savings from paired profiles."""
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def read(name):
    return json.loads((EXP / "results" / name).read_text())


def main():
    own = read("guarded_hires_profile.json")["games"]
    parent = read("v3_guard_profile_control.json")["games"]
    normalize = lambda lives: sorted((*life[:3], *life[5:]) for life in lives)
    changes = []
    for new, old in zip(own, parent, strict=True):
        assert (new["seed"], new["seat"]) == (old["seed"], old["seat"])
        assert new["produced"] == old["produced"]
        assert new["sold"] == old["sold"]
        a, b = new["profile"], old["profile"]
        assert normalize(a["lives"]) == normalize(b["lives"])
        assert (a["land"], a["land_cost"]) == (b["land"], b["land_cost"])
        saved_hire = b["hire_cost"] - a["hire_cost"]
        revenue = new["revenue"] - old["revenue"]
        other_spend = new["spend"] - old["spend"] + saved_hire
        assert new["cash"] - old["cash"] == revenue + saved_hire - other_spend
        changes.append({"seed": new["seed"], "seat": new["seat"],
            "cash_gain": new["cash"] - old["cash"], "saved_hires": b["hires"] - a["hires"],
            "saved_hire_cost": saved_hire, "revenue_change": revenue,
            "other_spend_change": other_spend,
            "discard_change": sum(new["discarded"]) - sum(old["discarded"]),
            "opponent_cash_change": new["opponent_cash"] - old["opponent_cash"]})
    means = {key: sum(row[key] for row in changes) / len(changes) for key in changes[0] if key not in {"seed", "seat"}}
    gate = {}
    for opponent in ["binghua_116", "john_128", "john_131"]:
        a = read(f"guard_new_counter_gate/guarded_hires_v3_vs_{opponent}.json")
        b = read(f"guard_new_counter_gate/opening_router_v3_vs_{opponent}.json")
        assert a["win_utility"] >= b["win_utility"]
        assert a["mean_margin"] >= b["mean_margin"]
        gate[opponent] = {"games": len(a["games"]), "utility": a["win_utility"],
            "parent_utility": b["win_utility"], "mean_margin": a["mean_margin"],
            "parent_margin": b["mean_margin"], "tail_margin": a["margin_cvar10"]}
    pass_j = {name: read(f"guard_pass_audit/{name}_vs_pass.json")["pass_J"]
              for name in ["guarded_hires_v3", "opening_router_v3"]}
    report = {"paired_profiles": len(changes), "equal_output_sales_daily_biology_land": True,
        "mean_changes": means, "per_game": changes, "fresh_new_counter_gate": gate,
        "pass_J_256_discovery": pass_j,
        "limits": "Physical guard only; extra wheat buy/discard in two paired games. Still loses most Binghua and Junghoon games."}
    (EXP / "results/guarded_hires_causal_validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: value for key, value in report.items() if key != "per_game"}, indent=2))


if __name__ == "__main__":
    main()
