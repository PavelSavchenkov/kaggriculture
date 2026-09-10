import argparse
import collections
import csv
import json
import statistics
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
ITEMS = ["wheat", "carrot", "tomato", "strawberry", "melon", "egg", "milk", "wool", "fertilizer", "goose", "cow", "sheep"]
OPS = ["none", "hire", "land", "seed", "product", "animal", "sell"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=EXP / "runs/top_patterns2315_v0")
    parser.add_argument("--manifest", type=Path, default=EXP / "research/refresh_sep09_2315/top_replay_manifest.csv")
    parser.add_argument("--calendars", type=Path, nargs="+", default=[EXP / "research/calendars_top2315/MANIFEST.json"])
    parser.add_argument("--episode-agents", type=Path, nargs="*", default=[])
    args = parser.parse_args()
    output = args.output
    manifest = list(csv.DictReader(args.manifest.open()))
    teams = {row["team"]: {"rank": int(row["rank"]), "team_id": row["team_id"], "submission_id": row["submission_id"]} for row in manifest}
    names = {r["episode"]: r["teams"] for path in args.calendars for r in json.loads(path.read_text())["cases"]}
    identities = {(e["id"], a["submissionId"], a["teamId"]): a.get("index", 0)
                  for path in args.episode_agents for e in json.loads(path.read_text())["episodes"] for a in e["agents"]}
    selected, resolved = {}, []
    for row in manifest:
        episode = int(row["episode_id"])
        key = episode, int(row["submission_id"]), int(row["team_id"])
        if key in identities:
            seat, method = identities[key], "episode submission/team IDs"
        else:
            assert names[episode].count(row["team"]) == 1, row
            seat, method = names[episode].index(row["team"]), "unique replay team name"
        assert (episode, seat) not in selected
        selected[episode, seat] = row["team"]
        resolved.append({"episode": episode, "seat": seat, "team": row["team"],
                         "replay_team": names[episode][seat], "method": method,
                         "submission_id": int(row["submission_id"]), "team_id": int(row["team_id"])})
    (output / "SELECTED_SEATS.json").write_text(json.dumps(resolved, indent=2, ensure_ascii=False) + "\n")
    grouped = {name: {"turn": [], "item": [], "order": []} for name in teams}
    for line in (output / "events.jsonl").open():
        row = json.loads(line)
        name = selected.get((row["episode"], row["seat"]))
        if name:
            grouped[name][row["kind"]].append(row)
    reports = []
    for name in sorted(teams, key=lambda x: teams[x]["rank"]):
        data = grouped[name]
        turns, items, orders = data["turn"], data["item"], data["order"]
        premium = [r for r in items if 0 < r["item"] < 8 and r["stock"] > 0]
        premium_sold = [r for r in premium if r["sold"] > 0]
        first_orders = [r for r in orders if r["slot"] == 0]
        report = {"team": name, **teams[name], "games": len({r["episode"] for r in turns}),
                  "requested_orders": sum(r["orders"] for r in turns),
                  "ineffective_orders": sum(r["ineffective"] for r in turns),
                  "ineffective_sales": sum(r["ineffective_sales"] for r in turns),
                  "zero_quantity_orders": sum(r["zero_quantity"] for r in turns),
                  "turns_with_orders": sum(r["orders"] > 0 for r in turns),
                  "turns_at_full_order_limit": sum(r["orders"] == 10 for r in turns),
                  "premium_stock_turns": len(premium), "premium_sale_turns": len(premium_sold),
                  "premium_sold_at_first_available_turn": sum(r["age"] == 0 for r in premium_sold),
                  "premium_waits_with_price_above_floor": sum(r["sold"] == 0 and r["price"] > 1 for r in premium),
                  "premium_waits_with_demand_due": sum(r["sold"] == 0 and r["demand_after"] > 0 for r in premium),
                  "premium_sells_before_demand": sum(r["sold"] > 0 and r["demand_after"] > 0 for r in premium),
                  "first_slot_executed_order_counts": dict(collections.Counter(OPS[r["op"]] for r in first_orders)),
                  "opening_orders": [], "products": {}, "purchases": {}}
        for episode in sorted({r["episode"] for r in turns}):
            opening = [r for r in orders if r["episode"] == episode and r["turn"] == 0]
            report["opening_orders"].append({"episode": episode, "orders": [[OPS[r["op"]], ITEMS[r["item"]] if r["item"] < len(ITEMS) else None, r["accepted"], r["cash_delta"]] for r in opening]})
        for item in range(9):
            x = [r for r in items if r["item"] == item]
            sales = [r for r in orders if r["op"] == 6 and r["item"] == item]
            buy = [r for r in orders if r["op"] == 4 and r["item"] == item]
            if not sales and not buy:
                continue
            report["products"][ITEMS[item]] = {"sold_units": sum(r["accepted"] for r in sales),
                "revenue": sum(r["cash_delta"] for r in sales), "sale_orders": len(sales),
                "sold_hours": dict(collections.Counter({str(h): sum(r["accepted"] for r in sales if r["turn"] % 24 == h) for h in range(24)})),
                "sale_slot_counts": dict(collections.Counter(r["slot"] for r in sales)),
                "observations_with_stock": sum(r["stock"] > 0 for r in x),
                "wait_observations_above_floor": sum(r["stock"] > 0 and r["sold"] == 0 and r["price"] > 1 for r in x),
                "wait_observations_at_floor": sum(r["stock"] > 0 and r["sold"] == 0 and r["price"] == 1 for r in x),
                "sale_observations_at_floor": sum(r["sold"] > 0 and r["price"] == 1 for r in x),
                "roundtrip_turns": [{"episode": r["episode"], "turn": r["turn"], "bought": r["bought"], "sold": r["sold"], "buy_slot": r["buy_slot"], "sale_slot": r["sale_slot"]} for r in x if r["sold"] and r["bought"]],
                "bought_units": sum(r["accepted"] for r in buy), "purchase_cost": -sum(r["cash_delta"] for r in buy)}
        for op in [3, 4, 5]:
            for item in range(12):
                buys = [r for r in orders if r["op"] == op and r["item"] == item]
                if buys:
                    report["purchases"][OPS[op] + "_" + ITEMS[item]] = {"orders": len(buys), "units": sum(r["accepted"] for r in buys),
                        "cost": -sum(r["cash_delta"] for r in buys), "turns": sorted({r["turn"] for r in buys}),
                        "quantities": dict(collections.Counter(r["accepted"] for r in buys))}
        reports.append(report)
    (output / "BY_TEAM.json").write_text(json.dumps(reports, indent=2, ensure_ascii=False) + "\n")
    for r in reports:
        print(json.dumps({k: r[k] for k in ["rank", "team", "requested_orders", "ineffective_orders", "zero_quantity_orders", "premium_stock_turns", "premium_sale_turns", "premium_sold_at_first_available_turn", "premium_waits_with_price_above_floor", "premium_sells_before_demand"]}, ensure_ascii=False))


if __name__ == "__main__":
    main()
