"""Summarize the frozen 23:15 replay discovery cohort; no policy code runs here."""
import collections
import csv
import json
import statistics
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]
RUN = EXP / "runs/top_patterns2315_v0"


def main():
    manifest = list(csv.DictReader((EXP / "research/refresh_sep09_2315/top_replay_manifest.csv").open()))
    names = {r["episode"]: r["teams"] for r in json.loads((EXP / "research/calendars_top2315/MANIFEST.json").read_text())["cases"]}
    selected = {(int(r["episode_id"]), names[int(r["episode_id"])].index(r["team"])): r["team"] for r in manifest}
    grouped = collections.defaultdict(lambda: collections.defaultdict(list))
    for line in (RUN / "events.jsonl").open():
        row = json.loads(line)
        name = selected.get((row["episode"], row["seat"]))
        if name in {"Otter Vibe", "binghua"}:
            grouped[(name, row["episode"], row["seat"], row["turn"])][row["kind"]].append(row)
    waits = []
    ordering = {}
    for key, data in grouped.items():
        for row in data["item"]:
            if 0 < row["item"] < 8 and row["stock"] and not row["sold"]:
                waits.append({"team": key[0], **row, "turn_info": data["turn"], "orders": data["order"]})
    for name in ["Otter Vibe", "binghua"]:
        counts = collections.Counter()
        for key, data in grouped.items():
            if key[0] != name:
                continue
            sales = sorted((r for r in data["item"] if r["sold"] and r["item"] < 9), key=lambda r: r["sale_slot"])
            if len(sales) < 2:
                continue
            counts["multi_product_turns"] += 1
            counts["item_ascending"] += all(a["item"] < b["item"] for a, b in zip(sales, sales[1:]))
            counts["price_descending"] += all(a["price"] >= b["price"] for a, b in zip(sales, sales[1:]))
            counts["revenue_descending"] += all(a["price"] * a["stock"] >= b["price"] * b["stock"] for a, b in zip(sales, sales[1:]))
        ordering[name] = dict(counts)
    details = {"waits": waits, "ordering": ordering}
    (RUN / "IMMEDIATE_DETAILS.json").write_text(json.dumps(details, indent=2) + "\n")

    fixed = collections.defaultdict(dict)
    for line in (EXP / "runs/top_patterns_fixed_v0/results.jsonl").open():
        row = json.loads(line)
        fixed[(row["episode"], row["seat"])][row["policy"]] = row
    team_effects = []
    teams = json.loads((RUN / "BY_TEAM.json").read_text())
    for team in teams:
        rows = [fixed[key] for key, name in selected.items() if name == team["team"]]
        effects = {"rank": team["rank"], "team": team["team"], "calendars": len(rows)}
        for policy in ["immediate_sales", "immediate_all", "sale_priority"]:
            x = [(r["room_keep"], r[policy]) for r in rows]
            feasible = lambda r: all(r[k] == 0 for k in ["own_missing", "rival_missing", "own_commitment_failures", "rival_commitment_failures"])
            effects[policy] = {"mean_margin_gain": statistics.mean(b["margin"] - a["margin"] for a, b in x),
                               "both_feasible": sum(feasible(a) and feasible(b) for a, b in x)}
        team_effects.append(effects)
    (RUN / "FIXED_PLAN_EFFECTS_BY_TEAM.json").write_text(json.dumps(team_effects, indent=2, ensure_ascii=False) + "\n")

    text = """# Latest top-player sales and purchases

Snapshot: September 9, 2026, 23:15 UTC. Three latest sampled appearances for each of the top 12 leaderboard teams: 36 selected player-games in 28 complete episodes. This is a small discovery sample. Shared schedules make several teams strongly related; it is not 12 independent policy families.

We reconstructed market execution in C++ and matched cash and market inventory after every turn. Counts below use stock after worker actions, when sales execute. A product-turn means one product in one turn; a turn holding three products contributes three observations. “Premium” here means carrot, tomato, strawberry, melon, egg, milk and wool; it excludes the purchasable inputs wheat and fertilizer.

## Observed patterns

| Rank | Team | Product-turns with a sale / premium stock present | Ineffective / requested market orders |
| --- | --- | ---: | ---: |
"""
    for r in teams:
        text += f'| {r["rank"]} | {r["team"]} | {r["premium_sale_turns"]} / {r["premium_stock_turns"]} | {r["ineffective_orders"]} / {r["requested_orders"]} |\n'
    text += """
Otter Vibe sells almost immediately. All 11 waits occurred on turns with ten occupied order slots; they are explained by the order limit. In all 206 turns with several products sold, their order follows descending stock quantity times current price. Sorting by price alone explains only 109/206. This points to a simple sale-value priority, although matching observations does not reveal the private code.

Binghua sells in all 439 premium-stock observations, including 225 sales just before known town/shop consumption. Its multi-product ordering does not follow the same sale-value rule. There is no universal “wait for the next price increase” pattern among the leaders.

Otter buys wheat mostly around day boundaries: 68 accepted purchase orders totaling 836 units across the three games. It buys 264 fertilizer in 30 orders. Binghua buys 301 wheat in 62 orders and 189 fertilizer in 33 orders. These policies combine immediate output sales with separate input purchasing; immediate output sales do not mean selling the feed reserve.

Several other teams repeat the same small purchases at the same turns despite different realized prices. Terry Luo, DeeperNet, mtmr_s1 and que la cuenten como quieran each make the same 20 fertilizer purchases per game: 46 units per game, using quantities one, two and four. This supports a fixed delivery schedule or shared lineage more strongly than price-sensitive purchasing. It does not prove the code is identical.

Several openings buy and resell wheat within one turn: examples include buy 13 / sell 13 / buy 13 and buy 13 / buy 60 / sell 60. Otter and Binghua show no same-turn wheat/fertilizer round trips in this sample. A round trip can change the opponent's intervening quotes even when its own direct profit is zero; it must be evaluated jointly, not deleted by assumption.

Many requested orders in the common schedule families execute nothing. Otter has none. This supports the already-tested narrow no-op compaction rule, but says nothing by itself about the value of moving purchases or hires.

## What transferred in tests

All live tests start from room_keep, our initial strong parent plus previously verified compaction and strict storage handling. Each first screen contains 640 paired comparisons: five opponents, 32 seeds, both seats, independent and native shop streams.

- Immediate premium selling on sale-only turns loses $3,798.69 mean margin. Applying it on all turns loses $3,438.42. Both change own production in 340 cases and labor in 605. Reject these live wrappers. In the traced PASS game, the baseline carries two wool into turn 168; the candidate has already sold them and has $398 more cash. The inherited schedule checks exact shed contents and switches its worker plan at that boundary. This is not an isolated timing comparison.
- Sorting existing sales by available quantity times current price gains $37.77 mean margin and 2.656 percentage points of win utility, with no production, labor or extra fault changes. The first-screen gain comes from Ahmed V25; three other active opponent means are negative. The subsequent frozen 12-opponent test on 64 new seeds and both shop panels gains $50.16 mean margin (seed-cluster 95% interval $33.91 to $66.17) and 1.660 percentage points of win utility over 3,072 paired comparisons. It has 1,302 positive and 969 negative comparisons, worst loss $752, with unchanged production/labor and no extra fault cases. Gains also occur against Yusuke, Junghoon and Atakan; Nanare, RouterV52, CapacityRouter and TITAN still lose on average. This supports keeping an experimental challenger, not a universal ordering claim.
- On the 56 fixed player calendars from all 28 episodes, immediate selling still loses: sale-only loses $1,135.81 mean margin among the 52 comparisons where both plans remain feasible; all-turn loses $881 among 44 feasible comparisons. Those feasible subsets are selected after evaluation and are descriptive only. Order priority gains $19.34 over all 56, all feasible. Opponent orders are fixed in this test, and both players' cash is recalculated.

The useful next step is to improve sale priority and calendar-aware timing while retaining input and schedule obligations. The latest leaders provide candidate rules; their rank is not a reason to copy a rule without a paired test.

## Evidence and reproduction

- `research/refresh_sep09_2315/top_replay_manifest.csv`: leaderboard ranks, team/submission IDs, episode IDs and collection times.
- `research/refresh_sep09_2315/DISCOVERY_SPLIT.json`: the latest top-12 families are now exposed for discovery, including previously reserved teams. Keep future evaluation families separate.
- `runs/top_patterns2315_v0/PROTOCOL.json`, `events.jsonl`, `BY_TEAM.json`, `IMMEDIATE_DETAILS.json`, `FIXED_PLAN_EFFECTS_BY_TEAM.json`: exact event data and summaries.
- `scripts/summarize_patterns.py`, `scripts/report_patterns.py`: offline summaries. Repeated market evaluation runs in C++.
- `runs/immediate_sales_screen_v0`, `runs/immediate_all_screen_v0`, `runs/sale_priority_screen_v0`, `runs/sale_priority_broad_v0`, `runs/sale_priority_checks_v0`, `runs/top_patterns_fixed_v0`: prospective protocols and complete results.

Leaderboard source: https://www.kaggle.com/competitions/kaggriculture/leaderboard
"""
    (EXP / "research/TOP_PATTERNS_2315.md").write_text(text)
    print("Wrote pattern report and per-team fixed-plan results.")


if __name__ == "__main__":
    main()
