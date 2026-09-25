"""Loads ledger CSVs (tools/ledger) and builds per-perspective aggregates.
import warnings
warnings.filterwarnings("ignore")

A perspective is one seat of one game. Top-team perspectives: seats whose label has a
leaderboard rank <= 10 ("Team|rank"). Our perspectives: label "ours:<variant>".
"""
import glob
import numpy as np
import pandas as pd

P = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer"]
C = P[:5]
A = ["goose", "cow", "sheep"]
PRODUCT = {"goose": "egg", "cow": "milk", "sheep": "wool"}
COST = {"goose": 300, "cow": 400, "sheep": 500}
SEED = {"wheat": 10, "carrot": 20, "tomato": 50, "strawberry": 100, "melon": 80}
BASE = {"wheat": 25, "carrot": 35, "tomato": 60, "strawberry": 120, "melon": 250, "egg": 50, "milk": 160, "wool": 200,
        "fertilizer": 100}
CATS = ["move", "plant", "water", "harvest_crop", "harvest_animal", "fertilize", "dig", "build", "place_animal", "shed_io",
        "feed", "care", "collect_fert", "idle"]


def load(pattern):
    out = {}
    for kind in ("games", "days", "crops", "animals"):
        files = sorted(glob.glob(f"{pattern}_{kind}.csv"))
        out[kind] = pd.concat([pd.read_csv(f) for f in files], ignore_index=True) if files else pd.DataFrame()
    return out


def perspectives(data, keep):
    """Per-perspective table. keep(label, opp_label, group) -> source name or None."""
    g = data["games"].copy()
    g["source"] = [keep(l, o, gr) for l, o, gr in zip(g.label, g.opp_label, g.group)]
    g = g[g.source.notna()].copy()
    key = ["trace", "seat"]
    d = data["days"].merge(g[key + ["source"]], on=key)
    d = d.copy()
    d["spend_animals"] = sum(d[f"bought_{a}"] * COST[a] for a in A)
    d["spend_seeds"] = d[[f"seed_cost_{c}" for c in C]].sum(axis=1)
    agg = {"revenue": "sum", "hire_cost": "sum", "land_cost": "sum", "spend_animals": "sum", "spend_seeds": "sum",
           "hires": "sum", "unit_turns": "sum", "cap_lost_crop": "sum", "bonus_lost": "sum", "fed": "sum", "cared": "sum",
           "unfed": "sum", "fert_missed": "sum", "night_carried": "sum", "discarded_animals": "sum"}
    for c in C:
        for f in ("planted", "seed_cost", "decay_lost", "weed_dry", "weed_dry_yield", "weed_decay"):
            agg[f"{f}_{c}"] = "sum"
    for a in A:
        for f in ("bought", "placed", "escapes", "cap_lost"):
            agg[f"{f}_{a}"] = "sum"
    for p in P:
        for f in ("sold", "rev", "bought", "buy_cost", "harvested", "discarded"):
            agg[f"{f}_{p}"] = "sum"
    for c in CATS:
        agg[f"act_{c}"] = "sum"
    for h in (0, 6, 12, 18):
        agg[f"sold_h{h}"] = "sum"
    s = d.groupby(key).agg(agg).reset_index()
    g = g.merge(s, on=key)
    g["margin"] = g.money - g.opp_money
    g["win"] = (g.margin > 0).astype(float)
    g["spend"] = g[["hire_cost", "land_cost", "spend_animals", "spend_seeds"]].sum(axis=1) + g[[f"buy_cost_{p}" for p in P]].sum(axis=1)
    for p in P:
        g[f"price_{p}"] = g[f"rev_{p}"] / g[f"sold_{p}"].replace(0, np.nan)
    # Sale timing: revenue relative to units x the day's mean market price.
    for p in P:
        d[f"ref_{p}"] = d[f"sold_{p}"] * d[f"mean_price_{p}"]
        d[f"refmax_{p}"] = d[f"sold_{p}"] * d[f"max_price_{p}"]
    r = d.groupby(key)[[f"ref_{p}" for p in P] + [f"refmax_{p}" for p in P]].sum().reset_index()
    g = g.merge(r, on=key)
    for p in P:
        g[f"timing_{p}"] = g[f"rev_{p}"] / g[f"ref_{p}"].replace(0, np.nan)
        g[f"timingmax_{p}"] = g[f"rev_{p}"] / g[f"refmax_{p}"].replace(0, np.nan)
    # Farm composition at chosen dawns.
    for day in (0, 3, 6, 9, 12, 15, 20, 25, 29):
        dd = d[d.day == day].set_index(key)
        for col in ["money_dawn", "land", "empty_owned", "weeds_owned", "empty_struct", "workers", "shed_dawn", "stock_value_dawn"] + \
                [f"plants_{c}" for c in C] + [f"animals_{a}" for a in A]:
            g[f"d{day}_{col}"] = g.set_index(key).index.map(dd[col]) if len(dd) else np.nan
    # Land purchase days.
    for q in (2, 3, 4):
        first = d[d.land >= q].groupby(key).day.min()
        g[f"land{q}_day"] = g.set_index(key).index.map(first)
    return g, d


def mean_table(g, cols, by="source"):
    return g.groupby(by)[cols].mean()
