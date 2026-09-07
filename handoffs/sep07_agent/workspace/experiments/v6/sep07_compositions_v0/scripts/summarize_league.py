"""Summarize saved full-game evidence; no search or policy code."""
import json
from pathlib import Path

EXP = Path(__file__).resolve().parents[1]


def main():
    rows = []
    for path in sorted((EXP / "results").glob("public_router_*_discovery.json")):
        result = json.loads(path.read_text())
        rows.append({"opponent": result["agent_b"], "games": len(result["games"]),
                     **{key: result[key] for key in ("win_utility", "mean_cash", "mean_margin", "margin_cvar10")}})
    promotion = json.loads((EXP / "results/public_router_teammate_promotion.json").read_text())
    wins = sum(g["cash"] > g["opponent_cash"] for g in promotion["games"])
    report = {"discovery": rows, "promotion_vs_teammate": {"games": len(promotion["games"]), "wins": wins,
              **{key: promotion[key] for key in ("win_utility", "mean_cash", "mean_margin", "margin_cvar10")}}}
    (EXP / "results/league_summary.json").write_text(json.dumps(report, indent=2) + "\n")
    for row in rows:
        print(row["opponent"], row["win_utility"], row["mean_margin"])
    print("promotion", report["promotion_vs_teammate"])


if __name__ == "__main__":
    main()
