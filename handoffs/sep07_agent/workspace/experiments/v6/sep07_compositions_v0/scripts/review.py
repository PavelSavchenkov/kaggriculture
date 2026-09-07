"""Summarize existing exact/replay evidence for a progress review."""
import argparse
import json
from pathlib import Path

import pandas as pd


EXPERIMENT = Path(__file__).resolve().parents[1]
RESEARCH = EXPERIMENT / "research"
PRODUCTS = ["WHEAT", "CARROT", "TOMATO", "STRAWBERRY", "MELON", "EGG", "MILK", "WOOL", "FERTILIZER"]


def main():
    global RESEARCH
    parser=argparse.ArgumentParser()
    parser.add_argument("--research-dir",type=Path,default=RESEARCH)
    args=parser.parse_args()
    RESEARCH=args.research_dir.resolve()
    assert RESEARCH.is_relative_to(EXPERIMENT)
    trades = pd.read_csv(RESEARCH / "transactions.csv")
    units = pd.read_csv(RESEARCH / "unit_events.csv")
    games = pd.read_csv(RESEARCH / "game_summary.csv")
    animals = pd.read_csv(RESEARCH / "animal_days.csv")
    crops = pd.read_csv(RESEARCH / "crop_days.csv")
    lives = pd.concat([pd.read_csv(RESEARCH / f"{kind}_instances.csv") for kind in ("animal", "crop")])
    records = []
    for game in games.itertuples():
        t = trades[(trades.episode_id == game.episode_id) & (trades.team == game.team)]
        u = units[(units.episode_id == game.episode_id) & (units.team == game.team)]
        a = animals[(animals.episode_id == game.episode_id) & (animals.team == game.team)]
        c = crops[(crops.episode_id == game.episode_id) & (crops.team == game.team)]
        l = lives[(lives.episode_id == game.episode_id) & (lives.team == game.team)]
        row = {"episode": game.episode_id, "team": game.team, "rank": game.rank, "cash": game.reward,
               "margin": game.reward - game.opponent_reward, "discards": game.drop_discards + game.dayend_discards}
        for product in PRODUCTS:
            bought = t[(t.operation == "BUY_PRODUCT") & (t.item == product)]
            sold = t[(t.operation == "SELL") & (t.item == product)]
            row[f"buy_{product}"] = int(bought.actual.sum())
            row[f"sell_{product}"] = int(sold.actual.sum())
            row[f"net_{product}"] = row[f"sell_{product}"] - row[f"buy_{product}"]
            row[f"buy_value_{product}"] = float(bought.value.sum())
            row[f"sale_value_{product}"] = float(sold.value.sum())
        for animal in ("COW", "SHEEP", "GOOSE"):
            selected = a[a.animal == animal]
            row[f"animal_days_{animal}"] = len(selected)
            for service in ("fed", "cared", "collected_fertilizer"):
                row[f"{service}_{animal}"] = float(selected[service].mean()) if len(selected) else None
            for day in (0, 2, 5, 8, 11, 17, 23, 29):
                row[f"{animal}_d{day}"] = int(((l.name == animal) & (l.origin_day <= day) & (l.end_day >= day)).sum())
        row["crop_days"] = len(c)
        row["crop_water_rate"] = float(c.watered.mean())
        relevant = c[c.yield_relevant == 1]
        row["crop_yield_day_maximized"] = float(relevant.maximized.mean())
        for crop in PRODUCTS[:5]:
            row[f"harvest_{crop}"] = int(u[(u.operation == "HARVEST") & (u.source == crop)].quantity.sum())
            row[f"plants_{crop}"] = int(((u.operation == "PLANT") & (u.source == crop)).sum())
        row["hires"] = int(t[t.operation == "HIRE"].actual.sum())
        row["hire_cost"] = float(t[t.operation == "HIRE"].value.sum())
        row["land_buys"] = int(t[t.operation == "BUY_LAND"].actual.sum())
        row["land_cost"] = float(t[t.operation == "BUY_LAND"].value.sum())
        row["weed_digs"] = int(((u.operation == "DIG") & (u.source.str.upper() == "WEED")).sum())
        if "unit_faults" in games.columns:
            row["unit_faults"] = game.unit_faults
            row["unit_requests"] = game.unit_requests
        for op in ("SELL", "BUY_PRODUCT"):
            s = t[t.operation == op]
            row[f"{op}_mean_hour"] = float((s.hour * s.actual).sum() / s.actual.sum())
        records.append(row)
    frame = pd.DataFrame(records)
    frame.to_csv(RESEARCH / "invariant_games.csv", index=False)
    frame.groupby(["rank", "team"]).mean(numeric_only=True).to_csv(RESEARCH / "invariant_teams.csv")
    numeric = frame.select_dtypes("number").mean().to_dict()
    summary = {"games": len(frame), "means": numeric}
    (RESEARCH / "invariant_means.json").write_text(json.dumps(summary, indent=2))
    keys = ["cash", "margin", "discards", "crop_days", "crop_water_rate", "crop_yield_day_maximized", "hires", "hire_cost", "land_buys"]
    print("Top-player means:", {key: round(numeric[key], 3) for key in keys})
    print("Gross buys:", {p: round(numeric[f"buy_{p}"], 1) for p in PRODUCTS})
    print("Net sales:", {p: round(numeric[f"net_{p}"], 1) for p in PRODUCTS})
    print("Crop harvest:", {p: round(numeric[f"harvest_{p}"], 1) for p in PRODUCTS[:5]})
    for name in ("teammate_pass_smoke", "teammate_self_smoke", "teammate_indar_discovery", "teammate_kaito_discovery", "teammate_boatlee_discovery"):
        result = json.loads((EXPERIMENT / "results" / f"{name}.json").read_text())
        print(name, {key: result[key] for key in ("win_utility", "mean_cash", "mean_margin", "margin_cvar10", "pass_J")})
        if name.endswith("smoke"):
            print("  mean produced:", [round(sum(g["produced"][i] for g in result["games"]) / len(result["games"]), 1) for i in range(9)])
            print("  mean faults/discards:", sum(g["unit_faults"] for g in result["games"])/len(result["games"]), sum(sum(g["discarded"]) for g in result["games"])/len(result["games"]))


if __name__ == "__main__":
    main()
