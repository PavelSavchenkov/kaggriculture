"""Describe executed public trading; no policy or source-family inference."""
import argparse
import json
from collections import Counter, defaultdict
from pathlib import Path


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    selected_rows = json.loads((args.run / "SELECTED_SEATS.json").read_text())
    selected = {(r["episode"], r["seat"]): r["team"] for r in selected_rows}
    grouped = defaultdict(lambda: defaultdict(list))
    for line in (args.run / "events.jsonl").open():
        row = json.loads(line)
        name = selected.get((row["episode"], row["seat"]))
        if name:
            grouped[name, row["episode"], row["seat"], row["turn"]][row["kind"]].append(row)
    counts = defaultdict(Counter)
    examples = []
    for (team, episode, seat, turn_index), data in grouped.items():
        assert len(data["turn"]) == 1
        turn = data["turn"][0]
        sales = sorted((r for r in data["item"] if r["item"] < 9 and r["sold"]), key=lambda r: r["sale_slot"])
        count = counts[team]
        if len(sales) > 1:
            count["multiple_sale_products"] += 1
            for key, value in [("value", lambda r: r["price"] * r["stock"]), ("quote", lambda r: r["price"])]:
                count[key + "_descending"] += all(value(a) >= value(b) for a, b in zip(sales, sales[1:]))
        for row in data["item"]:
            if not (0 < row["item"] < 8 and row["stock"] and not row["sold"]):
                continue
            count["wait_product_turns"] += 1
            count["wait_full_accepted_slots"] += turn["orders"] == 10 and turn["ineffective"] == 0
            count["wait_unused_or_failed_slot"] += turn["orders"] < 10 or turn["ineffective"] > 0
            count["wait_demand_due"] += row["demand_after"] > 0
            count["wait_rival_ready"] += row["rival_ready"] > 0
            count["wait_at_capacity"] += row["capacity_left"] == 0
            if team == "Otter Vibe":
                examples.append({"team": team, **row, "turn_info": turn})
    details = {"by_team": counts, "otter_waits": examples}
    (args.run / "BEHAVIOR_DETAILS.json").write_text(json.dumps(details, indent=2, ensure_ascii=False) + "\n")
    teams = json.loads((args.run / "BY_TEAM.json").read_text())
    text = f"""# Public sales and purchase patterns

This descriptive sample contains {len(selected_rows)} selected player-games in {len({r['episode'] for r in selected_rows})} episodes. Selection, source IDs and replay names are in SELECTED_SEATS.json; prospective data collection is in PROTOCOL.json. These episodes have already been used. They are not a fresh test for subsequent policy changes, and different team names do not prove independent code families.

Counts use C++ reconstructed execution and stock after worker actions. Cash and market inventory match every recorded turn. An output product-turn means one of carrot, tomato, strawberry, melon, egg, milk or wool in one turn. Several products in one turn count separately. The ordering comparison includes wheat and fertilizer and uses quotes and stock at the start of market execution.

| Team | Output sale / stock product-turns | Waits with ten successful orders / all waits | Ineffective / requested orders |
| --- | ---: | ---: | ---: |
"""
    for row in teams:
        c = counts[row["team"]]
        text += f"| {row['team']} | {row['premium_sale_turns']} / {row['premium_stock_turns']} | {c['wait_full_accepted_slots']} / {c['wait_product_turns']} | {row['ineffective_orders']} / {row['requested_orders']} |\n"
    text += "\n## What these observations support\n\n"
    for team in ("Otter Vibe", "binghua", "Unknown Mother-Goose", "Squirrel", "feel the agi"):
        if team not in counts:
            continue
        c = counts[team]
        text += (f"- {team}: {c['wait_product_turns']} output waits; {c['wait_full_accepted_slots']} have ten successful orders, "
                 f"{c['wait_unused_or_failed_slot']} have an unused or ineffective order slot. "
                 f"Quantity times current quote explains {c['value_descending']}/{c['multiple_sale_products']} multi-product sale turns; "
                 f"quote alone explains {c['quote_descending']}/{c['multiple_sale_products']}.\n")
    text += """
The sample contains both rapid sellers and frequent holders among strong teams. A full successful order list explains the immediate reason some products cannot be sold that turn. A wait with a free slot has another cause, but the replay alone does not reveal whether it is intentional, profitable or forced by a broader schedule. These observations propose tests; they do not establish optimality or the author's reasoning.

For our pipeline, compare the same funded farm and delivery calendar under alternative orders. Count stock capacity, required input deadlines, available order positions, current rival production and possible already harvested rival stock. Then check complete games and shared market effects after the edited window. A sales-friendly farm permits useful timing choices; an observed gain from repairing its existing orders is not proof of a better composition.

Team names can change: where episode metadata is supplied, stable submission and team IDs determine the seat. The original replay name remains recorded. Current source-program identity is stronger evidence than a name match, but it still does not establish code-family independence.

Reproduction: scripts/summarize_patterns.py builds BY_TEAM.json and SELECTED_SEATS.json; scripts/report_public_patterns.py builds this report and BEHAVIOR_DETAILS.json. All repeated financial simulation runs in C++.
"""
    (args.run / "SUMMARY.md").write_text(text)
    print(f"Wrote report for {len(teams)} teams and {len(selected_rows)} selected player-games.")


if __name__ == "__main__":
    main()
