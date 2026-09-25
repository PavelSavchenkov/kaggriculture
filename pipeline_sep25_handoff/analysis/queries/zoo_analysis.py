"""Zoo panel analysis: our candidate (base / x10 / x12) vs unpatched BC zoo models.
1. Outcome per matchup; 2. gap decomposition (ours - opponent) by product revenue and spend;
3. herd, wages, melon timing and sale hours for both sides; 4. won vs lost games within base.
usage: zoo_analysis.py   (after tools/ledger on ledger/zoo_list.txt -> ledger/zoo*)"""
import glob
import warnings
warnings.filterwarnings("ignore")
import numpy as np
import pandas as pd
from analyze import A, C, P, load, perspectives

pd.set_option("display.width", 250)


def source(label, opp, group):
    return label  # labels: ours:<cand> or opp:<model>


parts = [load(p) for p in sorted({f[:-len("_games.csv")] for f in glob.glob("ledger/zoo?_games.csv")})]
data = {k: pd.concat([x[k] for x in parts], ignore_index=True) for k in parts[0]}
g, d = perspectives(data, source)
g["cand"] = g.group.str.split("/").str[0]
g["model"] = g.group.str.split("/").str[1]
g["side"] = np.where(g.label.str.startswith("ours"), "ours", "opp")
g["animals15"] = sum(g[f"d15_animals_{a}"] for a in A)
g["animals9"] = sum(g[f"d9_animals_{a}"] for a in A)
g["animals1"] = sum(g[f"d3_animals_{a}"] for a in A)
d = d.merge(g[["trace", "seat", "side"]], on=["trace", "seat"])
g = g.set_index(["trace", "seat"])

ours = g[g.side == "ours"].copy()
opp = g[g.side == "opp"].copy()
opp.index = pd.MultiIndex.from_arrays([opp.index.get_level_values(0), 1 - opp.index.get_level_values(1)])
o = opp.reindex(ours.index)

print("== 1. outcome per matchup (ours = Vadim-opening candidate with flags)")
t = ours.groupby(["model", "cand"]).agg(n=("win", "size"), win=("win", "mean"), margin=("margin", "mean"), own=("money", "mean"), opp=("opp_money", "mean"))
print(t.unstack("cand").round(2).to_string())

print("\n== 2. gap ours - opponent, revenue by product and spend")
cols = [f"rev_{p}" for p in P] + ["spend_animals", "spend_seeds", "land_cost", "hire_cost", "buy_cost_wheat", "buy_cost_fertilizer"]
gap = ours[cols] - o[cols].values
gap["model"], gap["cand"] = ours.model, ours.cand
print(gap.groupby(["model", "cand"])[cols].mean().round(0).rename(columns=lambda c: c.replace("rev_", "")).to_string())

print("\n== 3. side by side: herd, melons, wages, sale hours")
cols3 = ["animals1", "animals9", "animals15", "bought_goose", "bought_cow", "bought_sheep", "planted_melon", "price_melon", "planted_strawberry",
         "planted_tomato", "planted_carrot", "planted_wheat", "hire_cost", "sold_h0", "sold_h18", "d29_land"]
side = pd.concat([ours[cols3].assign(who="ours", model=ours.model, cand=ours.cand), o[cols3].assign(who="opp", model=ours.model, cand=ours.cand)])
print(side.groupby(["model", "cand", "who"])[cols3].mean().round(1).to_string())

print("\n== 4. base: lost vs won games (ours), all zoo opponents; ours - opponent gaps")
b = ours[ours.cand == "base"].copy()
gb = gap[gap.cand == "base"]
b["lost"] = b.margin < 0
b["herd_gap"] = b.animals15 - o.loc[b.index, "animals15"].values
b["melon_gap"] = gb.loc[b.index, "rev_melon"]
cols4 = ["margin", "herd_gap", "melon_gap"] + [f"rev_{p}" for p in P]
x = pd.concat([b[["lost", "margin", "herd_gap", "melon_gap"]], gb.loc[b.index, [f"rev_{p}" for p in P]]], axis=1)
print(x.groupby("lost").mean().round(0).T.to_string())
print("\ncorrelation with margin:"); print(x.drop(columns="lost").corr()["margin"].round(2).to_string())
