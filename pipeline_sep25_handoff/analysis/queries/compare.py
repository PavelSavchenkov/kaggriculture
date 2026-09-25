"""Comparison tables: top-10 team perspectives vs our variants (and their opponents).
usage: compare.py <ledger prefix glob> [...]  e.g. compare.py 'ledger/top?' 'ledger/ours'
Prints plain-text tables; source = top (rank <= 10), top_vs20 (and opponent rank <= 20),
ours:<variant>, opp:<name>."""
import sys
import numpy as np
import pandas as pd
from analyze import A, C, P, CATS, COST, PRODUCT, load, perspectives

pd.set_option("display.width", 250)
pd.set_option("display.max_columns", 60)
pd.set_option("display.max_rows", 200)


def rank(label):
    return int(label.split("|")[1]) if "|" in label else 999


def source(label, opp, group):
    if label.startswith("ours:"):
        v, kind, opp_name = group.split("/")
        return f"ours:{v}:{'self' if opp_name == 'self' else kind}"
    if label.startswith("opp:"):
        return label
    if rank(label) <= 10:
        return "top_vs20" if rank(opp) <= 20 else "top"
    return None


def main():
    parts = [load(p) for p in sys.argv[1:]]
    data = {k: pd.concat([x[k] for x in parts], ignore_index=True) for k in parts[0]}
    g, d = perspectives(data, source)
    # Pool top and top_vs20 as "top_all" too.
    top_all = g[g.source.isin(["top", "top_vs20"])].copy()
    top_all["source"] = "top_all"
    g = pd.concat([g, top_all], ignore_index=True)
    order = [s for s in ["top_all", "top_vs20"] if s in set(g.source)] + sorted(s for s in set(g.source) if s.startswith("ours"))
    order += sorted(s for s in set(g.source) if s.startswith("opp"))
    G = g.groupby("source")

    def show(title, cols, fmt="{:,.1f}", rename=None):
        t = G[cols].mean().reindex(order)
        t.insert(0, "n", G.size().reindex(order))
        if rename:
            t = t.rename(columns=rename)
        print(f"\n## {title}\n")
        print(t.to_string(float_format=lambda v: fmt.format(v)))

    show("Outcome", ["money", "opp_money", "margin", "win", "revenue", "spend"], "{:,.2f}")
    show("Revenue by product ($/game)", [f"rev_{p}" for p in P], "{:,.0f}", {f"rev_{p}": p for p in P})
    show("Units sold by product", [f"sold_{p}" for p in P], "{:,.0f}", {f"sold_{p}": p for p in P})
    show("Average sale price by product", [f"price_{p}" for p in P], "{:,.1f}", {f"price_{p}": p for p in P})
    show("Sale timing: revenue / (units x day's mean price)", [f"timing_{p}" for p in P], "{:,.3f}", {f"timing_{p}": p for p in P})
    show("Sales by hour bucket (units)", ["sold_h0", "sold_h6", "sold_h12", "sold_h18"], "{:,.0f}")
    show("Spend ($/game)", ["spend_seeds", "spend_animals", "land_cost", "hire_cost", "buy_cost_wheat", "buy_cost_fertilizer"], "{:,.0f}")
    show("Purchased units", [f"bought_{a}" for a in A] + ["bought_wheat", "bought_fertilizer", "hires"], "{:,.1f}")
    show("Planted per crop (game)", [f"planted_{c}" for c in C], "{:,.1f}")
    show("Harvested/collected units by product", [f"harvested_{p}" for p in P], "{:,.0f}", {f"harvested_{p}": p for p in P})
    show("Land: day the n-th quadrant was owned", ["land2_day", "land3_day", "land4_day"], "{:,.2f}")
    for day in (0, 3, 6, 9, 12, 15, 20, 25, 29):
        show(f"Dawn of day {day}", [f"d{day}_{c}" for c in ["money_dawn", "land", "empty_owned", "weeds_owned", "empty_struct", "workers",
                                                              "shed_dawn", "stock_value_dawn"] + [f"plants_{x}" for x in C] + [f"animals_{a}" for a in A]],
             "{:,.1f}", {f"d{day}_{c}": c for c in ["money_dawn", "land", "empty_owned", "weeds_owned", "empty_struct", "workers", "shed_dawn",
                                                     "stock_value_dawn"] + [f"plants_{x}" for x in C] + [f"animals_{a}" for a in A]})
    loss = ["cap_lost_crop", "bonus_lost", "fert_missed", "discarded_animals", "night_carried"] + \
        [f"decay_lost_{c}" for c in C] + [f"weed_dry_{c}" for c in C] + [f"weed_decay_{c}" for c in C] + \
        [f"escapes_{a}" for a in A] + [f"cap_lost_{a}" for a in A] + [f"discarded_{p}" for p in P] + \
        ["unsold_units", "unsold_value", "carried_units", "held_units", "held_value"]
    show("Losses (units per game unless value)", loss, "{:,.1f}")
    show("Animal service", ["fed", "cared", "unfed"], "{:,.0f}")
    acts = [f"act_{c}" for c in CATS]
    shares = g[acts].div(g.unit_turns, axis=0)
    shares["source"] = g.source
    t = shares.groupby("source").mean().reindex(order)
    t["unit_turns"] = G.unit_turns.mean().reindex(order)
    print("\n## Worker time shares (successful actions / unit-turns)\n")
    print(t.rename(columns={f"act_{c}": c for c in CATS}).to_string(float_format=lambda v: f"{v:,.3f}"))

    # Daily flows by phase.
    d = d.merge(g[g.source != "top_all"][["trace", "seat"]].drop_duplicates(), on=["trace", "seat"])
    print("\n## Daily dawn money (median) by day\n")
    print(d.pivot_table(index="day", columns="source", values="money_dawn", aggfunc="median").reindex(columns=[o for o in order if o != "top_all"]).to_string(float_format=lambda v: f"{v:,.0f}"))
    d["animals"] = d[[f"animals_{a}" for a in A]].sum(axis=1)
    print("\n## Animals alive at dawn by day (mean)\n")
    print(d.pivot_table(index="day", columns="source", values="animals", aggfunc="mean").reindex(columns=[o for o in order if o != "top_all"]).to_string(float_format=lambda v: f"{v:,.1f}"))
    print("\n## Workers per day (mean)\n")
    print(d.pivot_table(index="day", columns="source", values="workers", aggfunc="mean").reindex(columns=[o for o in order if o != "top_all"]).to_string(float_format=lambda v: f"{v:,.1f}"))
    print("\n## Revenue per day (mean)\n")
    print(d.pivot_table(index="day", columns="source", values="revenue", aggfunc="mean").reindex(columns=[o for o in order if o != "top_all"]).to_string(float_format=lambda v: f"{v:,.0f}"))

    # Animal economics.
    an = data["animals"].merge(g[g.source != "top_all"][["trace", "seat", "source"]], on=["trace", "seat"])
    an["alive_nights"] = an.fed + an.unfed
    print("\n## Animals: per animal by species and placement phase\n")
    an["phase"] = pd.cut(an.day, [-1, 5, 11, 17, 23, 29], labels=["d0-5", "d6-11", "d12-17", "d18-23", "d24-29"])
    t = an.groupby(["species", "source", "phase"], observed=True).agg(n=("day", "size"), collected=("collected", "mean"),
                                                                     prod=("prod_units", "mean"), fed=("fed", "mean"),
                                                                     cared=("cared", "mean"), cap_lost=("cap_lost", "mean"),
                                                                     bonus_lost=("bonus_lost", "mean"), fert=("fert", "mean"),
                                                                     escaped=("escaped", lambda s: (s >= 0).mean()))
    nper = g[g.source != "top_all"].groupby("source").size()
    t["per_game"] = t.n / t.index.get_level_values("source").map(nper)
    print(t.to_string(float_format=lambda v: f"{v:,.2f}"))
    # Crop cohorts.
    cr = data["crops"].merge(g[g.source != "top_all"][["trace", "seat", "source"]], on=["trace", "seat"])
    cr["phase"] = pd.cut(cr.day, [-1, 2, 5, 11, 17, 23, 29], labels=["d0-2", "d3-5", "d6-11", "d12-17", "d18-23", "d24-29"])
    t = cr.groupby(["crop", "source", "phase"], observed=True)[["n", "harvested", "decay_lost", "cap_lost", "fate_harvested", "fate_dug",
                                                                 "fate_weed_dry", "fate_weed_decay", "fate_end"]].sum()
    t["per_plant"] = t.harvested / t.n
    t["n_per_game"] = t.n / t.index.get_level_values("source").map(nper)
    for f in ["decay_lost", "cap_lost"]:
        t[f] = t[f] / t.n
    for f in ["fate_harvested", "fate_dug", "fate_weed_dry", "fate_weed_decay", "fate_end"]:
        t[f] = t[f] / t.n
    print("\n## Crop cohorts: units harvested per plant and fates (shares)\n")
    print(t.drop(columns=["harvested"]).to_string(float_format=lambda v: f"{v:,.2f}"))


if __name__ == "__main__":
    main()
