"""Verify compiled route packages and separate labor savings from production."""
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def read(name):
    return json.loads((EXP / "results" / name).read_text())


def main():
    pairs = [("combine_hires_001_best_vs_opening_router_v2", "combine_hires_debug"),
             ("hire_day1_counter_vs_hire_day4_counter", "hire_day1_day4_debug"),
             ("hire_day9_counter_vs_pass", "hire_day9_debug")]
    checks = []
    for generic, debug in pairs:
        a = read(f"hire_routes_checks/{generic}.json")
        b = read(f"{debug}.json")
        assert a["games"] == b["games"]
        checks.append({"generic": generic, "debug": debug, "identical_games": len(a["games"])})
    parent = read("v2_self_hire_control.json")
    compiled = read("combine_hires_profile.json")
    # start/end are result-step timestamps: work at h23 is stamped at the next
    # day's h0. Born-day, occupied-day and service masks express daily biology.
    normalize = lambda lives: sorted((*l[:3], *l[5:]) for l in lives)
    for new, old in zip(compiled["games"], parent["games"], strict=True):
        assert (new["seed"], new["seat"]) == (old["seed"], old["seat"])
        assert new["cash"] == old["cash"] + 487
        assert new["opponent_cash"] == old["opponent_cash"]
        for key in ("produced", "sold", "discarded"):
            assert new[key] == old[key], key
        a, b = new["profile"], old["profile"]
        assert a["hires"] == b["hires"] - 5
        assert a["hire_cost"] == b["hire_cost"] - 487
        assert normalize(a["lives"]) == normalize(b["lives"])
        for key in ("buys", "land", "land_cost"):
            assert a[key] == b[key], key
    report = {"deployment": checks, "paired_profiles": len(compiled["games"]),
              "result": "All 32 games save exactly $487 and five hires; biological/service calendars, output, sales, purchases, land and discards unchanged."}
    generic = read("hire_combinations_checks/hire_day1_day9_vs_hire_day1_day4.json")
    debug = read("hire_combinations_debug.json")
    assert generic["games"] == debug["games"]
    report["combination_generic_debug_games"] = len(generic["games"])
    wrapper = read("opening_v3_wrapper_parity.json")
    original = read("hire_routes_promotion/hire_day1_day9_vs_opening_router_v2.json")
    assert wrapper["games"] == original["games"]
    generic = read("opening_v3_checks/opening_router_v3_vs_opening_router_v2.json")
    debug = read("opening_v3_debug.json")
    assert generic["games"] == debug["games"]
    arman = read("hire_routes_promotion/hire_day1_day9_vs_arman_3000.json")
    control = read("v2_arman_hire_control.json")
    assert arman["games"] == control["games"]
    report["v3_wrapper_equal_games"] = len(wrapper["games"])
    report["v3_generic_debug_equal_games"] = len(generic["games"])
    report["arman_parent_equal_games"] = len(arman["games"])
    (EXP / "results/hire_routes_validation.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
