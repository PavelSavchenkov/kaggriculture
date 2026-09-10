#!/usr/bin/env python3
"""Qualify learned seeds independently using closed-loop arena results."""

from __future__ import annotations

import argparse
import json
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("--prefix", required=True)
    parser.add_argument("--baseline", default="baseline-random")
    parser.add_argument("--minimum-score", type=float, default=0.50)
    parser.add_argument(
        "--minimum-promoted", type=int, default=0,
        help="required number of passing seeds; 0 retains the strict require-all behavior",
    )
    parser.add_argument(
        "--promotion-manifest", type=Path,
        help="write accepted/rejected agent IDs and their closed-loop evidence",
    )
    args = parser.parse_args()
    report = json.loads(args.report.read_text())
    records = report.get("game_records", [])
    learned = sorted(agent for agent in report.get("agents", [])
                     if agent.startswith(args.prefix + "-"))
    if not learned:
        raise SystemExit("no learned agents found in arena report")
    failures = []
    evaluations = {}
    accepted = []
    rejected = []
    print(f"[promotion gate] 0/{len(learned)} starting", flush=True)
    for index, agent in enumerate(learned, 1):
        games = [row for row in records if {row["challenger"], row["opponent"]}
                 == {agent, args.baseline}]
        if not games:
            failures.append(f"{agent}: no games against {args.baseline}")
            evaluations[agent] = {"accepted": False, "reason": "no baseline games"}
            rejected.append(agent)
            continue
        scores = []
        margins = []
        forfeits = 0
        for game in games:
            if game.get("forfeit"):
                forfeits += 1
                scores.append(0.0 if game["challenger"] == agent else 1.0)
                continue
            challenger_score = 1.0 if game["result"] == "win" else (
                0.5 if game["result"] == "draw" else 0.0)
            scores.append(challenger_score if game["challenger"] == agent
                          else 1.0 - challenger_score)
            money = game.get("money")
            if money and len(money) == 2:
                agent_seat = (int(game["seat"]) if game["challenger"] == agent
                              else 1 - int(game["seat"]))
                margins.append(float(money[agent_seat]) - float(money[1 - agent_seat]))
        score = sum(scores) / len(scores)
        mean_margin = sum(margins) / len(margins) if margins else None
        passed = score > args.minimum_score and forfeits == 0
        evaluations[agent] = {
            "accepted": passed,
            "score": score,
            "games": len(games),
            "wins": sum(value == 1.0 for value in scores),
            "draws": sum(value == 0.5 for value in scores),
            "losses": sum(value == 0.0 for value in scores),
            "mean_money_margin": mean_margin,
            "forfeits": forfeits,
        }
        detail = (f" mean_margin={mean_margin:+.1f}"
                  if mean_margin is not None else "")
        print(f"[promotion gate] {index}/{len(learned)} {agent} "
              f"score={score:.3f} games={len(games)}{detail}", flush=True)
        (accepted if passed else rejected).append(agent)
    required = len(learned) if args.minimum_promoted == 0 else args.minimum_promoted
    if required < 1 or required > len(learned):
        raise SystemExit(f"--minimum-promoted must be between 1 and {len(learned)}, or 0")
    summary = {
        "arena_report": str(args.report),
        "baseline": args.baseline,
        "minimum_score_exclusive": args.minimum_score,
        "minimum_promoted": required,
        "accepted": accepted,
        "rejected": rejected,
        "evaluations": evaluations,
    }
    if args.promotion_manifest:
        args.promotion_manifest.parent.mkdir(parents=True, exist_ok=True)
        args.promotion_manifest.write_text(json.dumps(summary, indent=2) + "\n")
        print(f"[promotion manifest] {args.promotion_manifest}", flush=True)
    if len(accepted) < required:
        failures.append(f"only {len(accepted)} of {len(learned)} seeds passed; "
                        f"at least {required} required")
    if report.get("forfeits", 0):
        failures.append(f"arena recorded {report['forfeits']} forfeits")
    if failures:
        raise SystemExit("promotion rejected:\n  " + "\n  ".join(failures))
    print(f"[promotion gate] PASS promoted={len(accepted)}/{len(learned)}", flush=True)


if __name__ == "__main__":
    main()
