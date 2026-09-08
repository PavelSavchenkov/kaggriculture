"""Promote only from complete paired league, native and operational evidence."""
import hashlib
import json
import math
from pathlib import Path
from statistics import mean

import numpy as np

EXP = Path(__file__).resolve().parents[1]
NEW = "crop_value_m2_t4"
OLD = "investment_context_guarded_001_best"
FRESH = EXP / "runs/crop_value_validation_001"
CHECKS = EXP / "runs/crop_value_checks_001"


def read(path):
    return json.loads(path.read_text())


def summary(data):
    games = data["games"]
    margins = sorted(g["cash"] - g["opponent_cash"] for g in games)
    tail = math.ceil(len(games) / 10)
    wins, ties = sum(m > 0 for m in margins), margins.count(0)
    return {"games": len(games), "wins": wins, "ties": ties, "losses": len(games) - wins - ties,
            "utility": (wins + ties / 2) / len(games), "margin": mean(margins),
            "margin_cvar10": mean(margins[:tail]), "pass_J": data["pass_J"]}


def utility(game):
    return float(game["cash"] > game["opponent_cash"]) + .5 * (game["cash"] == game["opponent_cash"])


def main():
    groups = read(EXP / "results/investment_context_validation.json")["grouping"]
    checks = read(CHECKS / "CHECKS.json")
    assert checks["generic_pair_debug_thread_all_game_records_equal"]
    records = {}
    raw = {}
    for agent in [OLD, NEW]:
        raw[agent] = {p.stem.split("_vs_", 1)[1]: read(p) for p in FRESH.glob(agent + "_vs_*.json")}
        raw[agent]["shop_herd_guarded_001_best"] = read(CHECKS / (agent + "_group_missing.json"))
        records[agent] = {opponent: summary(data) for opponent, data in raw[agent].items()}
    paired = {}
    for opponent, data in raw[NEW].items():
        a, b = data["games"], raw[OLD][opponent]["games"]
        assert len(a) == 1024 and [(g["seed"], g["seat"]) for g in a] == [(g["seed"], g["seat"]) for g in b]
        delta = [x["cash"] - x["opponent_cash"] - y["cash"] + y["opponent_cash"] for x, y in zip(a, b)]
        gain = records[NEW][opponent]["utility"] - records[OLD][opponent]["utility"]
        assert mean(delta) >= 0 and gain >= 0, opponent
        paired[opponent] = {"utility_gain": gain, "mean_margin_gain": mean(delta),
                            "margin_better": sum(d > 0 for d in delta), "margin_worse": sum(d < 0 for d in delta),
                            "margin_tail_gain": records[NEW][opponent]["margin_cvar10"] - records[OLD][opponent]["margin_cvar10"]}
    scores = {agent: mean(mean(records[agent][b]["utility"] for b in opponents) for opponents in groups.values()) for agent in [OLD, NEW]}
    assert scores[NEW] > scores[OLD]
    # Resample complete seeds across opponents and seats, preserving dependence.
    by_seed = {seed: 0.0 for seed in range(1360000, 1360512)}
    for opponents in groups.values():
        for opponent in opponents:
            for a, b in zip(raw[NEW][opponent]["games"], raw[OLD][opponent]["games"]):
                by_seed[a["seed"]] += (utility(a) - utility(b)) / (2 * len(groups) * len(opponents))
    values = np.array(list(by_seed.values()))
    rng = np.random.default_rng(710)
    bootstrap = values[rng.integers(0, len(values), size=(4000, len(values)))].mean(axis=1)
    interval = np.quantile(bootstrap, [.025, .975]).tolist()
    assert interval[0] > 0
    native = {}
    for opponent in [OLD, "teammate_shoprouter", "king_rc4", "public_router", "public_router_v5"]:
        native[opponent] = {agent: summary(read(CHECKS / (agent + "_native_" + opponent + ".json"))) for agent in [OLD, NEW]}
        assert native[opponent][NEW]["utility"] >= native[opponent][OLD]["utility"]
        assert native[opponent][NEW]["margin"] >= native[opponent][OLD]["margin"]
    pass_results = {agent: summary(read(CHECKS / (agent + "_pass256.json"))) for agent in [OLD, NEW]}
    assert pass_results[NEW]["pass_J"] >= pass_results[OLD]["pass_J"]
    a = read(CHECKS / (NEW + "_profile64.json"))["games"]
    b = read(CHECKS / (OLD + "_profile64.json"))["games"]
    for x, y in zip(a, b):
        assert x["profile"]["hires"] == y["profile"]["hires"] and x["profile"]["hire_cost"] == y["profile"]["hire_cost"]
        assert x["unit_faults"] == y["unit_faults"] and x["discarded"] == y["discarded"]
        delta = tuple(x["produced"][i] - y["produced"][i] for i in range(9))
        assert delta in [(0,) * 9, (0, 0, 0, 2, 0, 0, 0, 0, 0)]
    causal = {"games": len(a), "changed": sum(x["action_hash"] != y["action_hash"] for x, y in zip(a, b)),
              "mean_own_cash_gain": mean(x["cash"] - y["cash"] for x, y in zip(a, b)),
              "mean_rival_cash_gain": mean(x["opponent_cash"] - y["opponent_cash"] for x, y in zip(a, b)),
              "labor_faults_discards_unchanged": True, "only_production_change": "+2STRAW in each activated game"}
    files = [EXP / "include/crop_value_gate.hpp", EXP / "include/guarded_sequence.hpp",
             EXP / "runs/crop_value_001/proposals" / NEW / "source/agent.hpp", Path(__file__).resolve(),
             EXP / "scripts/check_crop_value.py", EXP / "runs/crop_value_001/PARITY.json"]
    report = {"candidate": NEW, "parent": OLD, "status": "Promoted as local search reference after exact paired and operational checks",
              "scope": "Fresh1360000..1360511,both seats,15opponents; native1420000..1420127,both seats. CPU C++ policies only.",
              "objective": "Unchanged equal group mean win utility; cash margin and lower tail reported separately",
              "grouping": groups, "group_objective": scores, "seed_cluster_bootstrap_gain_95pct": interval,
              "fresh": records, "paired": paired, "native": native, "pass": pass_results, "causal": causal,
              "operational_checks": str((CHECKS / "CHECKS.json").relative_to(EXP)),
              "lineage": "Justin source150 and all investment_context_guarded_001_best components, one V30 day20fertilization+checked continuation from fertilization004_0, local observed strawberry-demand gate >=4 at day20hour0. No future shops or private rival state.",
              "source_sha256": {str(p.relative_to(EXP)): hashlib.sha256(p.read_bytes()).hexdigest() for p in files},
              "limits": ["Small crop-service improvement; general multi-family composition and robust dependency rebuilding remain incomplete.",
                         "Some individual paired margins decline; direct-version lower-CVaR10 margin declines about$5 despite increased utility and mean.",
                         "Two independent inherited controllers add runtime work; operational tests pass, submission adapter not built for this variant.",
                         "User's newly requested submission is exactly investment_context_guarded_001_best; this crop gate is not included in that submission."]}
    output = EXP / "results/crop_value_validation.json"
    output.write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"group_objective": scores, "gain_95pct": interval, "causal": causal}, indent=2))


if __name__ == "__main__":
    main()
