#!/usr/bin/env python3
"""Select exact behavior-clone lineages using Kaggle leaderboard ratings.

The Kaggle Episode API exposes ``submissionId``, ``initialScore``, and
``updatedScore`` for both seats. A submission, not a team display name, is the
policy-version identity: one team can upload many materially different agents.

This tool joins those rows to local ``.kagz`` episodes, ranks each submission by
its latest observed Kaggle rating, and writes an exact cluster manifest for
``train_seed_models.py``. Legacy rating files without submission IDs are
rejected unless ``--allow-team-fallback`` is explicitly requested.
"""

from __future__ import annotations

import argparse
import collections
import csv
import json
from pathlib import Path
import statistics
import sys
import time


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "corpus"))
import kagz_format  # noqa: E402


def log(message: str) -> None:
    print(message, file=sys.stderr, flush=True)


def _number(value, cast=float):
    if value is None or value == "":
        return None
    return cast(value)


def load_ratings(path: Path) -> dict[int, dict[int, dict]]:
    """Load API CSV or legacy joined JSON, indexed by episode and seat."""
    indexed: dict[int, dict[int, dict]] = collections.defaultdict(dict)
    if path.suffix.lower() == ".csv":
        with path.open(newline="") as handle:
            records = list(csv.DictReader(handle))
        for record in records:
            episode = int(record["episode_id"])
            for seat in (0, 1):
                updated = _number(record.get(f"updatedScore{seat}"))
                initial = _number(record.get(f"initialScore{seat}"))
                score = updated if updated is not None else initial
                if score is None:
                    continue
                indexed[episode][seat] = {
                    "episode": episode,
                    "seat": seat,
                    "submission_id": _number(record.get(f"submissionId{seat}"), int),
                    "team_id": _number(record.get(f"teamId{seat}"), int),
                    "team": record.get(f"teamName{seat}") or None,
                    "score": score,
                    "initial_score": initial,
                    "updated_score": updated,
                    "create_time": record.get("create_time") or "",
                    "cash": _number(record.get(f"reward{seat}")),
                    "opp_cash": _number(record.get(f"reward{1 - seat}")),
                }
        return indexed

    records = json.loads(path.read_text())
    if not isinstance(records, list):
        raise ValueError(f"rating JSON must contain a list: {path}")
    for record in records:
        episode = int(record.get("episode", record.get("episode_id")))
        seat = int(record["seat"])
        submission = record.get("submission_id", record.get("submissionId"))
        score = record.get("score", record.get("updatedScore",
                                                record.get("initialScore")))
        if score is None:
            continue
        indexed[episode][seat] = {
            "episode": episode,
            "seat": seat,
            "submission_id": _number(submission, int),
            "team_id": _number(record.get("team_id", record.get("teamId")), int),
            "team": record.get("team") or record.get("team_name"),
            "score": float(score),
            "initial_score": _number(record.get("initial_score",
                                                 record.get("initialScore"))),
            "updated_score": _number(record.get("updated_score",
                                                 record.get("updatedScore"))),
            "create_time": record.get("create_time", record.get("createTime", "")),
            "cash": _number(record.get("cash", record.get("reward"))),
            "opp_cash": _number(record.get("opp_cash", record.get("opponent_reward"))),
        }
    return indexed


def latest_submission_ratings(ratings: dict[int, dict[int, dict]]) -> dict[int, dict]:
    """Return each submission's latest rating over the complete API crawl."""
    histories: dict[int, list[tuple[str, int, float]]] = collections.defaultdict(list)
    for episode, seats in ratings.items():
        for row in seats.values():
            submission = row.get("submission_id")
            if submission:
                histories[int(submission)].append(
                    (row.get("create_time") or "", episode, float(row["score"])))
    result = {}
    for submission, points in histories.items():
        points.sort(key=lambda point: (point[0], point[1]))
        values = [point[2] for point in points]
        result[submission] = {
            "elo": values[-1], "elo_first": values[0],
            "elo_change": values[-1] - values[0],
            "elo_range": max(values) - min(values),
            "rating_observations": len(values),
        }
    return result


def summarize(replays: Path, ratings: dict[int, dict[int, dict]],
              allow_team_fallback: bool = False) -> tuple[dict[str, dict], dict]:
    """Join rated trajectories and aggregate by exact submission identity."""
    lineages: dict[str, dict] = {}
    counters = collections.Counter()
    current_ratings = latest_submission_ratings(ratings)
    paths = sorted(replays.glob("*.kagz"))
    started = time.time()
    log(f"[join ratings] 0/{len(paths)} (  0.0%) starting")
    report_every = max(1, len(paths) // 20)
    for index, path in enumerate(paths, 1):
        if index % report_every == 0 or index == len(paths):
            elapsed = time.time() - started
            log(f"[join ratings] {index}/{len(paths)} ({100*index/max(1,len(paths)):5.1f}%) "
                f"elapsed={elapsed:.1f}s rate={index/max(elapsed,1e-9):.1f}/s")
        try:
            episode = int(path.stem)
        except ValueError:
            counters["invalid_filename"] += 1
            continue
        seats = ratings.get(episode)
        if not seats:
            counters["unrated_episodes"] += 1
            continue
        packed = kagz_format.read(path)
        for seat in (0, 1):
            row = seats.get(seat)
            if row is None:
                counters["unrated_participants"] += 1
                continue
            team = row.get("team") or packed["teams"][seat]
            submission = row.get("submission_id")
            if submission:
                identity = f"submission:{submission}"
            elif allow_team_fallback:
                identity = f"team:{team}"
                counters["team_fallback_participants"] += 1
            else:
                counters["missing_submission_participants"] += 1
                continue
            cash = row.get("cash")
            opponent_cash = row.get("opp_cash")
            if cash is None:
                cash = float(packed["rewards"][seat])
            if opponent_cash is None:
                opponent_cash = float(packed["rewards"][1 - seat])
            entry = lineages.setdefault(identity, {
                "identity": identity,
                "submission_id": submission,
                "team_ids": set(),
                "team_counts": collections.Counter(),
                "trajectories": [],
                "wins": 0,
                "draws": 0,
                "losses": 0,
                "margins": [],
            })
            if row.get("team_id") is not None:
                entry["team_ids"].add(int(row["team_id"]))
            entry["team_counts"][team] += 1
            entry["trajectories"].append({"path": str(path), "seat": seat})
            margin = float(cash) - float(opponent_cash)
            entry["wins"] += margin > 0
            entry["losses"] += margin < 0
            entry["draws"] += margin == 0
            entry["margins"].append(margin)
            counters["rated_participants"] += 1
    for entry in lineages.values():
        team = sorted(entry.pop("team_counts").items(), key=lambda row: (-row[1], row[0]))[0][0]
        entry["team"] = team
        entry["cluster"] = (f"{team} [submission {entry['submission_id']}]"
                            if entry["submission_id"] else team)
        entry["team_ids"] = sorted(entry["team_ids"])
        if entry["submission_id"]:
            entry.update(current_ratings[int(entry["submission_id"])])
        else:
            # Explicit legacy fallback: its rating rows have no version identity.
            points = [(row.get("create_time") or "", episode, float(row["score"]))
                      for episode, seats in ratings.items() for row in seats.values()
                      if row.get("team") == team]
            points.sort(key=lambda point: (point[0], point[1]))
            values = [point[2] for point in points]
            entry.update({"elo": values[-1], "elo_first": values[0],
                          "elo_change": values[-1] - values[0],
                          "elo_range": max(values) - min(values),
                          "rating_observations": len(values)})
        entry["win_rate"] = entry["wins"] / len(entry["trajectories"])
        entry["mean_margin"] = round(statistics.fmean(entry["margins"]), 1)
        entry["median_margin"] = round(statistics.median(entry["margins"]), 1)
        del entry["margins"]
    return lineages, dict(counters)


def describe(entry: dict) -> str:
    submission = entry["submission_id"] if entry["submission_id"] else "team-fallback"
    return (f"{entry['team'][:23]:23s} sub={str(submission):>8s} "
            f"elo={entry['elo']:7.1f} n={len(entry['trajectories']):5d} "
            f"win={entry['win_rate']:6.1%} margin={entry['mean_margin']:+9.0f}")


def public_entry(entry: dict) -> dict:
    return {key: value for key, value in entry.items() if key != "trajectories"}


def select_ranked(eligible: list[dict], maximum: int,
                  allow_multiple_per_team: bool) -> list[dict]:
    ranked = sorted(eligible, key=lambda entry: (-entry["elo"],
                                                -len(entry["trajectories"]),
                                                entry["identity"]))
    if allow_multiple_per_team:
        return ranked[:maximum]
    selected, used_teams = [], set()
    for entry in ranked:
        if entry["team"] in used_teams:
            continue
        selected.append(entry)
        used_teams.add(entry["team"])
        if len(selected) == maximum:
            break
    return selected


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--ratings", type=Path,
                        default=Path("corpus/index/episode_agents.csv"))
    parser.add_argument("--replays", type=Path, default=Path("corpus/data_all"))
    parser.add_argument("--max-seeds", type=int, default=4)
    parser.add_argument("--submission-id", type=int, action="append",
                        help="require this exact Kaggle submission; repeatable")
    parser.add_argument("--min-elo", type=float, default=2600.0)
    parser.add_argument("--min-trajectories", type=int, default=40,
                        help="eligibility floor per exact submission")
    parser.add_argument("--min-margin", type=float)
    parser.add_argument("--allow-team-fallback", action="store_true",
                        help="explicitly permit legacy rating rows without submission IDs")
    parser.add_argument("--allow-multiple-per-team", action="store_true")
    parser.add_argument("--manifest", type=Path,
                        help="write exact episode/seat cluster assignments for the trainer")
    parser.add_argument("--out", type=Path, help="write the full selection report")
    args = parser.parse_args()

    if not args.ratings.is_file():
        raise SystemExit(f"ratings file not found: {args.ratings}")
    if not args.replays.is_dir():
        raise SystemExit(f"replay directory not found: {args.replays}")
    ratings = load_ratings(args.ratings)
    has_submission_identity = any(
        row.get("submission_id") for seats in ratings.values() for row in seats.values())
    if not has_submission_identity and not args.allow_team_fallback:
        raise SystemExit(
            "rating data contains no submissionId values; use an Episode API CSV from "
            "corpus/kaggle_index.py, or explicitly pass --allow-team-fallback")
    lineages, coverage = summarize(args.replays, ratings, args.allow_team_fallback)
    if coverage.get("missing_submission_participants"):
        log(f"[select] skipped {coverage['missing_submission_participants']} rated "
            "participants without submission identity")
    log(f"[select] {len(lineages)} rated policy lineages; coverage={coverage}")
    requested = set(args.submission_id or [])
    if requested:
        found = {int(entry["submission_id"]) for entry in lineages.values()
                 if entry["submission_id"]}
        missing = sorted(requested - found)
        if missing:
            raise SystemExit(f"requested submission IDs have no mapped replays: {missing}")
    eligible = [entry for entry in lineages.values()
                if len(entry["trajectories"]) >= args.min_trajectories
                and entry["elo"] >= args.min_elo
                and (not requested or entry["submission_id"] in requested)
                and (args.min_margin is None or entry["mean_margin"] >= args.min_margin)]
    selected = select_ranked(eligible, args.max_seeds, args.allow_multiple_per_team)
    ranked = sorted(eligible, key=lambda entry: (-entry["elo"], entry["identity"]))
    if len(selected) < args.max_seeds:
        raise SystemExit(
            f"only {len(selected)} distinct eligible lineages satisfy elo>={args.min_elo:.0f} "
            f"and n>={args.min_trajectories}")

    log(f"[select] selected {len(selected)} exact lineages by latest Kaggle rating:")
    for entry in selected:
        log(f"[select]   {describe(entry)}")
    selected_ids = {entry["identity"] for entry in selected}
    for entry in [row for row in ranked if row["identity"] not in selected_ids][:3]:
        log(f"[select]   (next) {describe(entry)}")

    report = {
        "ranking_key": "latest observed Kaggle updatedScore by submissionId",
        "identity_policy": "submissionId; team fallback disabled" if not args.allow_team_fallback
                           else "submissionId with explicit team fallback",
        "eligibility": {"min_elo": args.min_elo,
                        "min_trajectories": args.min_trajectories,
                        "min_margin": args.min_margin,
                        "requested_submission_ids": sorted(requested),
                        "distinct_teams": not args.allow_multiple_per_team},
        "replays": str(args.replays),
        "ratings": str(args.ratings),
        "coverage": coverage,
        "rated_lineages": len(lineages),
        "eligible_lineages": len(eligible),
        "selected": [public_entry(entry) for entry in selected],
        "runners_up": [public_entry(entry) for entry in ranked
                       if entry["identity"] not in selected_ids][:5],
    }
    if args.out:
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n")
        log(f"[select] wrote {args.out}")
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        manifest = {
            "format": 1,
            "source": str(args.ratings),
            "identity": "Kaggle submissionId",
            "clusters": {entry["cluster"]: entry["trajectories"] for entry in selected},
        }
        args.manifest.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n")
        log(f"[select] wrote exact trainer manifest {args.manifest}")

    for entry in selected:
        print(entry["cluster"])


if __name__ == "__main__":
    main()
