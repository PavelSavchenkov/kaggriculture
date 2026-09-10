#!/usr/bin/env python3
"""Summarize fresh LocalLB results for self-play descendants and their parents."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import tempfile


def descendant_result(record: dict, descendant: str) -> tuple[float, float | None]:
    """Return score and terminal-money margin from the descendant's perspective."""
    challenger = record["challenger"]
    result = record["result"]
    if result == "draw":
        score = 0.5
    else:
        challenger_won = result == "win"
        score = float(challenger_won == (challenger == descendant))
    money = record.get("money")
    if money is None or any(value is None for value in money):
        return score, None
    challenger_seat = int(record["seat"])
    descendant_seat = challenger_seat if challenger == descendant else 1 - challenger_seat
    return score, float(money[descendant_seat] - money[1 - descendant_seat])


def summarize(arena: dict, exports: dict) -> dict:
    ratings = arena["ratings"]
    ordered = sorted(ratings, key=lambda agent: (-ratings[agent], agent))
    ranks = {agent: index for index, agent in enumerate(ordered, 1)}
    rows = []
    for exported in exports["descendants"]:
        parent = exported["parent_agent"]
        descendant = exported["descendant_agent"]
        if parent not in ratings or descendant not in ratings:
            raise ValueError(f"arena roster is missing parent/descendant pair: {parent}, {descendant}")
        pair = [record for record in arena["game_records"]
                if {record["challenger"], record["opponent"]} == {parent, descendant}]
        scores_and_margins = [descendant_result(record, descendant) for record in pair]
        scores = [score for score, _margin in scores_and_margins]
        margins = [margin for _score, margin in scores_and_margins if margin is not None]
        stats = arena["stats"][descendant]
        direct_score = sum(scores) / len(scores) if scores else None
        direct_margin = sum(margins) / len(margins) if margins else None
        rows.append({
            **exported,
            "descendant_rank": ranks[descendant],
            "descendant_rating": ratings[descendant],
            "parent_rank": ranks[parent],
            "parent_rating": ratings[parent],
            "rating_delta_vs_parent": ratings[descendant] - ratings[parent],
            "overall_wins": stats[0],
            "overall_losses": stats[1],
            "overall_draws": stats[2],
            "direct_games_vs_parent": len(scores),
            "direct_score_vs_parent": direct_score,
            "direct_mean_money_margin_vs_parent": direct_margin,
            "directly_better_than_parent": bool(
                direct_score is not None and
                (direct_score > 0.5 or
                 (direct_score == 0.5 and direct_margin is not None and direct_margin > 0))),
        })
    return {
        "format": 1,
        "arena_games": arena["games"],
        "arena_forfeits": arena["forfeits"],
        "arena_seconds": arena["seconds"],
        "agents": len(arena["agents"]),
        "descendants": rows,
    }


def atomic_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
            "w", dir=path.parent, prefix=f".{path.name}.", delete=False) as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")
        temporary = Path(stream.name)
    temporary.replace(path)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--arena", type=Path, required=True)
    parser.add_argument("--exports", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = summarize(json.loads(args.arena.read_text()),
                       json.loads(args.exports.read_text()))
    atomic_json(args.output, result)
    print("\nSelf-play descendant comparison")
    print(f"{'descendant':<42} {'rank':>5} {'rating':>8} {'delta':>8} "
          f"{'vs-parent':>10} {'margin':>10}")
    for row in result["descendants"]:
        score = row["direct_score_vs_parent"]
        margin = row["direct_mean_money_margin_vs_parent"]
        print(f"{row['descendant_agent']:<42} {row['descendant_rank']:>5} "
              f"{row['descendant_rating']:>8.1f} {row['rating_delta_vs_parent']:>+8.1f} "
              f"{score:>10.3f} {margin:>+10.1f}")
    print(f"[self-play evaluation report] {args.output.resolve()}", flush=True)


if __name__ == "__main__":
    main()
